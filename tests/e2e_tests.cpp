#include <gtest/gtest.h>
#include "wasm3_instance.hpp"
#include "plugin.hpp"

// We include the generated trampoline header!
#include "host_classes.hpp"
#include "plugin_trampoline.hpp"

using namespace cerridwen;

TEST(CerridwenE2E, BasicWasmExecution) {
    // 1. Load the Wasm3 instance
    std::string wasm_path = "plugin.wasm";
    if (FILE* f = fopen(wasm_path.c_str(), "rb")) {
        fclose(f);
    } else if (FILE* f2 = fopen("tests/plugin.wasm", "rb")) {
        wasm_path = "tests/plugin.wasm";
        fclose(f2);
    } else {
        wasm_path = "build/tests/plugin.wasm";
    }
    auto instance = std::make_unique<Wasm3Instance>(wasm_path);
    
    // We must bind host functions BEFORE calling any WASM functions
    // because Wasm3 resolves imports during the first compilation.
    std::cout << "[Test] Binding LoggerHost to WASM module...\n";
    bind_LoggerHost(instance->get_module()); // Generated binder
    
    // 2. Call the constructor function exported by WASM
    uint32_t wasm_ptr = instance->call_create("MathPlugin_create");
    ASSERT_NE(wasm_ptr, 0);
    
    // 3. Create the Trampoline proxy object on the C++ side
    MathPlugin_Trampoline trampoline;
    trampoline._wasm_instance = instance.get();
    trampoline._wasm_ptr = wasm_ptr;
    
    // 4. Test state interaction using value marshalling
    std::cout << "[Test] Initial sync...\n";
    trampoline.sync_counter();
    EXPECT_EQ(trampoline.counter, 0);
    
    // 5. Invoke virtual methods that trampoline proxies to WASM
    std::cout << "[Test] Calling increment() first time...\n";
    trampoline.increment();
    std::cout << "[Test] After increment, counter=" << trampoline.counter << "\n";
    EXPECT_EQ(trampoline.counter, 1);
    
    std::cout << "[Test] Calling increment() second time...\n";
    trampoline.increment();
    std::cout << "[Test] After increment 2, counter=" << trampoline.counter << "\n";
    EXPECT_EQ(trampoline.counter, 2);
    
    std::cout << "[Test] Calling decrement()...\n";
    trampoline.decrement();
    EXPECT_EQ(trampoline.counter, 1);
    
    std::cout << "[Test] Calling multiply(5, 4)...\n";
    int product = trampoline.multiply(5, 4);
    EXPECT_EQ(product, 20);
    
    // 6. Test Host-to-Plugin RPC (Upcalls)
    LoggerHost my_logger;
    uint64_t host_ptr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&my_logger));
    
    std::cout << "[Test] Calling do_logging() with host_ptr=" << host_ptr << " (hex: " << std::hex << host_ptr << std::dec << ")...\n";
    trampoline.do_logging(host_ptr);
    EXPECT_EQ(trampoline.counter, 123456789);
}
