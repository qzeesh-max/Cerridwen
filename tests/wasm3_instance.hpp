#pragma once

#include <cerridwen/plugin_manager.hpp>
#include <wasm3.h>
#include <m3_env.h>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace cerridwen {

class Wasm3Instance : public WasmInstance {
    IM3Environment env;
    IM3Runtime runtime;
    IM3Module module = nullptr;
    std::vector<uint8_t> _wasm_bytes;

public:
    Wasm3Instance(const std::string& wasm_path) {
        env = m3_NewEnvironment();
        if (!env) throw std::runtime_error("m3_NewEnvironment failed");
        
        runtime = m3_NewRuntime(env, 65536, NULL);
        if (!runtime) throw std::runtime_error("m3_NewRuntime failed");
        
        std::ifstream file(wasm_path, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open WASM file");
        
        _wasm_bytes = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        M3Result result = m3_ParseModule(env, &module, _wasm_bytes.data(), _wasm_bytes.size());
        if (result) throw std::runtime_error(std::string("m3_ParseModule failed: ") + result);
        
        result = m3_LoadModule(runtime, module);
        if (result) throw std::runtime_error(std::string("m3_LoadModule failed: ") + result);
    }
    
    ~Wasm3Instance() override {
        if (runtime) m3_FreeRuntime(runtime);
        if (env) m3_FreeEnvironment(env);
    }
    
    IM3Module get_module() const { return module; }

    void call(const std::string& func_name, uint32_t ptr) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
        if (result) throw std::runtime_error(std::string("m3_FindFunction failed for ") + func_name + ": " + result);
        
        const char* argv[1];
        std::string ptr_str = std::to_string(ptr);
        argv[0] = ptr_str.c_str();
        
        result = m3_CallArgv(func, 1, argv);
        if (result) throw std::runtime_error(std::string("m3_Call failed for ") + func_name + ": " + result);
    }
    
    void call_args(const std::string& func_name, const std::vector<std::string>& args) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
        if (result) throw std::runtime_error(std::string("m3_FindFunction failed for ") + func_name + ": " + result);
        
        std::vector<const char*> argv;
        for (const auto& s : args) argv.push_back(s.c_str());
        
        result = m3_CallArgv(func, argv.size(), argv.data());
        if (result) throw std::runtime_error(std::string("m3_Call failed for ") + func_name + ": " + result);
    }
    
    int call_args_int(const std::string& func_name, const std::vector<std::string>& args) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
        if (result) throw std::runtime_error(std::string("m3_FindFunction failed for ") + func_name + ": " + result);
        
        std::vector<const char*> argv;
        for (const auto& s : args) argv.push_back(s.c_str());
        
        result = m3_CallArgv(func, argv.size(), argv.data());
        if (result) throw std::runtime_error(std::string("m3_Call failed for ") + func_name + ": " + result);
        
        int32_t ret = 0;
        const void* ret_ptrs[1] = { &ret };
        m3_GetResults(func, 1, ret_ptrs);
        return ret;
    }
    
    int call_int(const std::string& func_name, uint32_t ptr) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
        if (result) throw std::runtime_error(std::string("m3_FindFunction failed for ") + func_name + ": " + result);
        
        const char* argv[1];
        std::string ptr_str = std::to_string(ptr);
        argv[0] = ptr_str.c_str();
        
        result = m3_CallArgv(func, 1, argv);
        if (result) throw std::runtime_error(std::string("m3_Call failed for ") + func_name + ": " + result);
        
        int32_t ret = 0;
        const void* ret_ptrs[1] = { &ret };
        m3_GetResults(func, 1, ret_ptrs);
        return ret;
    }
    
    void call_set_int(const std::string& func_name, uint32_t ptr, int val) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
        if (result) throw std::runtime_error(std::string("m3_FindFunction failed for ") + func_name + ": " + result);
        
        const char* argv[2];
        std::string ptr_str = std::to_string(ptr);
        std::string val_str = std::to_string(val);
        argv[0] = ptr_str.c_str();
        argv[1] = val_str.c_str();
        
        result = m3_CallArgv(func, 2, argv);
        if (result) throw std::runtime_error(std::string("m3_Call failed for ") + func_name + ": " + result);
    }
    
    uint32_t allocate(size_t size) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, "malloc");
        if (result) throw std::runtime_error("malloc not found in wasm");
        
        const char* argv[1];
        std::string size_str = std::to_string(size);
        argv[0] = size_str.c_str();
        
        result = m3_CallArgv(func, 1, argv);
        if (result) throw std::runtime_error("malloc call failed");
        
        uint32_t ret = 0;
        const void* ret_ptrs[1] = { &ret };
        m3_GetResults(func, 1, ret_ptrs);
        return ret;
    }
    
    void deallocate(uint32_t ptr) override {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, "free");
        if (result) throw std::runtime_error("free not found in wasm");
        
        const char* argv[1];
        std::string ptr_str = std::to_string(ptr);
        argv[0] = ptr_str.c_str();
        
        m3_CallArgv(func, 1, argv);
    }
    
    void read_memory(uint32_t wasm_ptr, void* dest, size_t size) override {
        size_t mem_size = 0;
        uint8_t* mem = m3_GetMemory(module, &mem_size, 0);
        if (!mem || wasm_ptr + size > mem_size) throw std::runtime_error("read_memory out of bounds");
        std::memcpy(dest, mem + wasm_ptr, size);
    }
    
    void write_memory(uint32_t wasm_ptr, const void* src, size_t size) override {
        size_t mem_size = 0;
        uint8_t* mem = m3_GetMemory(module, &mem_size, 0);
        if (!mem || wasm_ptr + size > mem_size) throw std::runtime_error("write_memory out of bounds");
        std::memcpy(mem + wasm_ptr, src, size);
    }
    
    uint32_t call_create(const std::string& func_name) {
        IM3Function func;
        M3Result result = m3_FindFunction(&func, runtime, func_name.c_str());
        if (result) {
            M3ErrorInfo info;
            m3_GetErrorInfo(runtime, &info);
            std::string extra = info.message ? info.message : "";
            throw std::runtime_error(std::string("m3_FindFunction failed for ") + func_name + ": " + result + " | Details: " + extra);
        }
        
        result = m3_CallArgv(func, 0, nullptr);
        if (result) {
            M3ErrorInfo info;
            m3_GetErrorInfo(runtime, &info);
            std::string extra = info.message ? info.message : "";
            throw std::runtime_error(std::string("m3_Call failed for ") + func_name + ": " + result + " | Details: " + extra);
        }
        
        uint32_t ret = 0;
        const void* ret_ptrs[1] = { &ret };
        m3_GetResults(func, 1, ret_ptrs);
        return ret;
    }
};

} // namespace cerridwen
