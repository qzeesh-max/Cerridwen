#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <iostream>

namespace cerridwen {

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
    
    // Generic argument calls
    virtual void call_args(const std::string& func_name, const std::vector<std::string>& args) { throw std::runtime_error("Not implemented"); }
    virtual int call_args_int(const std::string& func_name, const std::vector<std::string>& args) { throw std::runtime_error("Not implemented"); }

    template <typename BaseClass>
    std::unique_ptr<BaseClass> instantiate(const std::string& className) {
        std::cout << "[WasmInstance] Mock instantiating class: " << className << "\n";
        return nullptr;
    }
};

// Plugin Manager to load and manage WASM modules
class PluginManager {
public:
    std::shared_ptr<WasmInstance> load_plugin(const std::string& path) {
        // Mock loading
        std::cout << "[PluginManager] Loading mock plugin: " << path << "\n";
        return std::make_shared<WasmInstance>();
    }
};

} // namespace cerridwen
