#pragma once
#include <cstdint>
#include <cerridwen/annotations.hpp>

struct CERRIDWEN_EXPORT_WASM CERRIDWEN_THREAD_SAFE MathPlugin {
    virtual ~MathPlugin() = default;
    
    virtual void increment() { this->counter++; }
    virtual void decrement() { this->counter--; }
    virtual int multiply(int a, int b) { return a * b; }
    
    // Upcall test
    virtual void do_logging(uint64_t logger_ptr);
    
    CERRIDWEN_MARSHAL_BY_VALUE
    int counter = 0;
};

#ifdef __EMSCRIPTEN__
#include "plugin_host_proxies.hpp"
inline void MathPlugin::do_logging(uint64_t logger_ptr) {
    LoggerHost_Proxy logger(logger_ptr);
    logger.log_message("Hello from WASM Plugin!");
    int ts = logger.get_timestamp();
    this->counter = ts; // store it to verify
}
#else
inline void MathPlugin::do_logging(uint64_t logger_ptr) {}
#endif
