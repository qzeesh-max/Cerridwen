#pragma once

#include <cerridwen/errors.hpp>
#include <cerridwen/plugin_manager.hpp>
#include <cerridwen/storage.hpp>

#include <wasm3.h>
#include <m3_env.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace cerridwen {

struct InstanceOptions {
    // Hard ceiling (bytes) on the plugin's linear memory, enforced by the host no
    // matter what the plugin asks for. 0 = no host-imposed ceiling (the plugin's
    // own declared maximum still applies).
    size_t max_memory_bytes = 0;
    // Wasm3 value-stack size in bytes.
    uint32_t stack_size = 64 * 1024;
};

// A plugin instance backed by Wasm3. Not copyable: imports bound into the module
// keep a pointer back to the instance.
class Wasm3Instance : public WasmInstance {
    IM3Environment env = nullptr;
    IM3Runtime runtime = nullptr;
    IM3Module module = nullptr;
    IM3Function _alloc_func = nullptr;
    IM3Function _free_func = nullptr;
    std::unordered_map<std::string, IM3Function> _func_cache;
    
    std::vector<uint8_t> _wasm_bytes;
    StorageHost* _storage = nullptr;
    size_t _memory_limit = 0;
    int _depth = 0;

    struct DepthGuard {
        Wasm3Instance& i;
        explicit DepthGuard(Wasm3Instance& inst) : i(inst) {
            if (i._depth++ == 0) take_pending_error();
        }
        ~DepthGuard() { --i._depth; }
    };

    // ---- Host imports every plugin may use (module "env") -------------------------

    // void cerridwen_throw(const char* message): raise an error from the plugin.
    static m3ApiRawFunction(import_throw) {
        m3ApiGetArg(uint32_t, msg_off);
        std::string msg;
        if (!read_wasm_string(_mem, msg_off, msg, 4096)) msg = "plugin error (unreadable message)";
        record_plugin_error(std::move(msg));
        m3ApiTrap("cerridwen:plugin_exception");
    }

    static Wasm3Instance* self(IM3ImportContext ctx) { return static_cast<Wasm3Instance*>(ctx->userdata); }

    // int storage_write(const char* name, const void* data, uint32_t len)
    static m3ApiRawFunction(import_storage_write) {
        m3ApiReturnType(int32_t);
        m3ApiGetArg(uint32_t, name_off);
        m3ApiGetArg(uint32_t, data_off);
        m3ApiGetArg(uint32_t, len);
        StorageHost* st = self(_ctx)->_storage;
        if (!st) m3ApiReturn(StorageHost::NoStorage);
        std::string name;
        if (!read_wasm_string(_mem, name_off, name, StorageHost::kMaxNameLength + 1)) m3ApiReturn(StorageHost::InvalidName);
        m3ApiCheckMem(m3ApiOffsetToPtr(data_off), len);
        m3ApiReturn(st->write(name, m3ApiOffsetToPtr(data_off), len));
    }

    // int storage_read(const char* name, void* buf, uint32_t cap): returns blob size or <0
    static m3ApiRawFunction(import_storage_read) {
        m3ApiReturnType(int32_t);
        m3ApiGetArg(uint32_t, name_off);
        m3ApiGetArg(uint32_t, buf_off);
        m3ApiGetArg(uint32_t, cap);
        StorageHost* st = self(_ctx)->_storage;
        if (!st) m3ApiReturn(StorageHost::NoStorage);
        std::string name;
        if (!read_wasm_string(_mem, name_off, name, StorageHost::kMaxNameLength + 1)) m3ApiReturn(StorageHost::InvalidName);
        m3ApiCheckMem(m3ApiOffsetToPtr(buf_off), cap);
        int64_t r = st->read(name, m3ApiOffsetToPtr(buf_off), cap);
        m3ApiReturn(static_cast<int32_t>(r > INT32_MAX ? INT32_MAX : r));
    }

    // int storage_size(const char* name)
    static m3ApiRawFunction(import_storage_size) {
        m3ApiReturnType(int32_t);
        m3ApiGetArg(uint32_t, name_off);
        StorageHost* st = self(_ctx)->_storage;
        if (!st) m3ApiReturn(StorageHost::NoStorage);
        std::string name;
        if (!read_wasm_string(_mem, name_off, name, StorageHost::kMaxNameLength + 1)) m3ApiReturn(StorageHost::InvalidName);
        int64_t r = st->size(name);
        m3ApiReturn(static_cast<int32_t>(r > INT32_MAX ? INT32_MAX : r));
    }

    // int storage_remove(const char* name)
    static m3ApiRawFunction(import_storage_remove) {
        m3ApiReturnType(int32_t);
        m3ApiGetArg(uint32_t, name_off);
        StorageHost* st = self(_ctx)->_storage;
        if (!st) m3ApiReturn(StorageHost::NoStorage);
        std::string name;
        if (!read_wasm_string(_mem, name_off, name, StorageHost::kMaxNameLength + 1)) m3ApiReturn(StorageHost::InvalidName);
        m3ApiReturn(st->remove(name));
    }

    // uint64 storage_used() / storage_quota()
    static m3ApiRawFunction(import_storage_used) {
        m3ApiReturnType(uint64_t);
        StorageHost* st = self(_ctx)->_storage;
        m3ApiReturn(st ? st->used() : 0);
    }
    static m3ApiRawFunction(import_storage_quota) {
        m3ApiReturnType(uint64_t);
        StorageHost* st = self(_ctx)->_storage;
        m3ApiReturn(st ? st->quota() : 0);
    }

    // Reads a NUL-terminated string at `off`, bounded by both linear memory and max_len.
    static bool read_wasm_string(void* mem, uint32_t off, std::string& out, size_t max_len) {
        size_t sz = m3_GetMemorySizeAt(mem);
        if (off >= sz) return false;
        size_t n = std::min(max_len, sz - off);
        const char* p = static_cast<const char*>(mem) + off;
        const void* z = std::memchr(p, 0, n);
        if (!z) return false;
        out.assign(p, static_cast<const char*>(z) - p);
        return true;
    }

    void link_import(const char* name, const char* sig, M3RawCall fn) {
        M3Result r = m3_LinkRawFunctionEx(module, "env", name, sig, fn, this);
        if (r && r != m3Err_functionLookupFailed) {
            throw std::runtime_error(std::string("m3_LinkRawFunctionEx failed for ") + name + ": " + r);
        }
    }

    void link_runtime_imports() {
        link_import("cerridwen_throw", "v(i)", &import_throw);
        link_import("cerridwen_storage_write", "i(iii)", &import_storage_write);
        link_import("cerridwen_storage_read", "i(iii)", &import_storage_read);
        link_import("cerridwen_storage_size", "i(i)", &import_storage_size);
        link_import("cerridwen_storage_remove", "i(i)", &import_storage_remove);
        link_import("cerridwen_storage_used", "I()", &import_storage_used);
        link_import("cerridwen_storage_quota", "I()", &import_storage_quota);
    }

    // Converts a failed wasm3 call into the right typed C++ exception.
    [[noreturn]] void raise(M3Result result, const std::string& what) {
        PendingError pe = take_pending_error();
        if (pe.kind == ErrorKind::Host) throw HostException(pe.message);
        if (pe.kind == ErrorKind::Plugin) throw PluginException(pe.message);
        if (result == m3Err_memoryLimitExceeded) throw ResourceLimitError(what + ": " + result);
        M3ErrorInfo info;
        m3_GetErrorInfo(runtime, &info);
        std::string extra = info.message ? info.message : "";
        throw PluginTrap(what + ": " + result + (extra.empty() ? "" : " | Details: " + extra));
    }

    IM3Function invoke(const std::string& func_name, const std::vector<std::string>& args) {
        DepthGuard guard(*this);
        IM3Function func = nullptr;
        auto it = _func_cache.find(func_name);
        if (it != _func_cache.end()) {
            func = it->second;
        } else {
            M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
            if (result) raise(result, "m3_FindFunction failed for " + func_name);
            _func_cache[func_name] = func;
        }
        std::vector<const char*> argv;
        for (const auto& s : args) argv.push_back(s.c_str());
        M3Result result = m3_CallArgv(func, static_cast<uint32_t>(argv.size()), argv.data());
        if (result) raise(result, "m3_Call failed for " + func_name);
        return func;
    }

    static int32_t result_i32(IM3Function func) {
        int32_t ret = 0;
        const void* ptrs[1] = {&ret};
        m3_GetResults(func, 1, ptrs);
        return ret;
    }

public:
    explicit Wasm3Instance(const std::string& wasm_path, InstanceOptions opts = {})
        : _memory_limit(opts.max_memory_bytes) {
        env = m3_NewEnvironment();
        if (!env) throw std::runtime_error("m3_NewEnvironment failed");

        runtime = m3_NewRuntime(env, opts.stack_size, nullptr);
        if (!runtime) throw std::runtime_error("m3_NewRuntime failed");
        runtime->memoryBytesLimit = opts.max_memory_bytes;

        std::ifstream file(wasm_path, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open WASM file: " + wasm_path);
        _wasm_bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        M3Result result = m3_ParseModule(env, &module, _wasm_bytes.data(), _wasm_bytes.size());
        if (result) throw std::runtime_error(std::string("m3_ParseModule failed: ") + result);

        result = m3_LoadModule(runtime, module);
        if (result) {
            if (result == m3Err_memoryLimitExceeded)
                throw ResourceLimitError(std::string("plugin's initial memory exceeds host limit: ") + result);
            throw std::runtime_error(std::string("m3_LoadModule failed: ") + result);
        }
        link_runtime_imports();
    }

    Wasm3Instance(const Wasm3Instance&) = delete;
    Wasm3Instance& operator=(const Wasm3Instance&) = delete;

    ~Wasm3Instance() override {
        if (runtime) m3_FreeRuntime(runtime);
        if (env) m3_FreeEnvironment(env);
    }

    IM3Module get_module() const { return module; }

    // Gives the plugin access to a host-controlled storage area. Pass nullptr to revoke.
    void set_storage(StorageHost* storage) { _storage = storage; }

    size_t memory_bytes() const override { return m3_GetMemorySize(module, 0); }
    size_t memory_limit() const override { return _memory_limit; }

    void call(const std::string& func_name, uint32_t ptr) override { invoke(func_name, {std::to_string(ptr)}); }

    int call_int(const std::string& func_name, uint32_t ptr) override {
        return result_i32(invoke(func_name, {std::to_string(ptr)}));
    }

    void call_set_int(const std::string& func_name, uint32_t ptr, int val) override {
        invoke(func_name, {std::to_string(ptr), std::to_string(val)});
    }

    void call_args(const std::string& func_name, const std::vector<std::string>& args) override {
        invoke(func_name, args);
    }

    int call_args_int(const std::string& func_name, const std::vector<std::string>& args) override {
        return result_i32(invoke(func_name, args));
    }

    uint32_t allocate(size_t size) override {
        if (!_alloc_func) {
            M3Result result = m3_FindFunction(&_alloc_func, runtime, "cerridwen_alloc");
            if (result) throw std::runtime_error("cerridwen_alloc not found");
        }
        const void* args[1];
        uint32_t arg_size = size;
        args[0] = &arg_size;
        DepthGuard guard(*this);
        M3Result result = m3_Call(_alloc_func, 1, args);
        if (result) raise(result, "cerridwen_alloc failed");
        uint32_t ptr = static_cast<uint32_t>(result_i32(_alloc_func));
        if (ptr == 0) throw ResourceLimitError("plugin malloc failed (out of memory or limit reached)");
        return ptr;
    }

    void deallocate(uint32_t ptr) override {
        if (!_free_func) {
            M3Result result = m3_FindFunction(&_free_func, runtime, "cerridwen_free");
            if (result) throw std::runtime_error("cerridwen_free not found");
        }
        const void* args[1];
        args[0] = &ptr;
        DepthGuard guard(*this);
        M3Result result = m3_Call(_free_func, 1, args);
        if (result) raise(result, "cerridwen_free failed");
    }

    void read_memory(uint32_t wasm_ptr, void* dest, size_t size) override {
        size_t mem_size = 0;
        uint8_t* mem = m3_GetMemory(module, &mem_size, 0);
        if (!mem || static_cast<uint64_t>(wasm_ptr) + size > mem_size) throw std::runtime_error("read_memory out of bounds");
        std::memcpy(dest, mem + wasm_ptr, size);
    }

    void write_memory(uint32_t wasm_ptr, const void* src, size_t size) override {
        size_t mem_size = 0;
        uint8_t* mem = m3_GetMemory(module, &mem_size, 0);
        if (!mem || static_cast<uint64_t>(wasm_ptr) + size > mem_size) throw std::runtime_error("write_memory out of bounds");
        std::memcpy(mem + wasm_ptr, src, size);
    }

    uint32_t call_create(const std::string& func_name) {
        return static_cast<uint32_t>(result_i32(invoke(func_name, {})));
    }
};

} // namespace cerridwen
