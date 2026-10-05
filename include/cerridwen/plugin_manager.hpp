#pragma once

#include <cerridwen/errors.hpp>
#include <cstdint>
#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include <vector>

namespace cerridwen {

// Helper to share a std::shared_ptr with WASM.
// Returns a 64-bit handle that should be passed to WASM functions expecting a SharedProxy.
template<typename T>
uint64_t share_to_wasm(std::shared_ptr<T> ptr) {
    if (!ptr) return 0;
    auto* heap_sp = new std::shared_ptr<T>(ptr);
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(heap_sp));
}

// Forward declaration of a WASM Instance
class WasmInstance {
public:
    virtual ~WasmInstance() = default;

    virtual void call(const std::string& func_name, uint32_t ptr) { throw std::runtime_error("Not implemented"); }
    virtual int call_int(const std::string& func_name, uint32_t ptr) { throw std::runtime_error("Not implemented"); }
    virtual void call_set_int(const std::string& func_name, uint32_t ptr, int val) { throw std::runtime_error("Not implemented"); }
    virtual void read_memory(uint32_t wasm_ptr, void* dest, size_t size) { throw std::runtime_error("Not implemented"); }
    virtual void write_memory(uint32_t wasm_ptr, const void* src, size_t size) { throw std::runtime_error("Not implemented"); }
    virtual uint32_t allocate(size_t size) { throw std::runtime_error("Not implemented"); }
    virtual void deallocate(uint32_t ptr) { throw std::runtime_error("Not implemented"); }

    // Linear memory introspection (bytes currently committed / host-imposed ceiling, 0 = none).
    virtual size_t memory_bytes() const { throw std::runtime_error("Not implemented"); }
    virtual size_t memory_limit() const { return 0; }
    
    // Generic argument calls
    virtual void call_args(const std::string& func_name, const std::vector<std::string>& args) { throw std::runtime_error("Not implemented"); }
    virtual int call_args_int(const std::string& func_name, const std::vector<std::string>& args) { throw std::runtime_error("Not implemented"); }

    template <typename BaseClass>
    std::unique_ptr<BaseClass> instantiate(std::string_view className) {
        std::cout << "[WasmInstance] Mock instantiating class: " << className << "\n";
        return nullptr;
    }
};

// Plugin Manager to load and manage WASM modules
class PluginManager {
public:
    std::shared_ptr<WasmInstance> load_plugin(std::string_view path) {
        // Mock loading
        std::cout << "[PluginManager] Loading mock plugin: " << path << "\n";
        return std::make_shared<WasmInstance>();
    }
};

} // namespace cerridwen
