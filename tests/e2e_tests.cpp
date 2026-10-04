#include <gtest/gtest.h>
#include "cerridwen/wasm3_instance.hpp"
#include "plugin.hpp"
#include "cerridwen/plugin_manager.hpp"

// We include the generated trampoline header!
#include "host_classes.hpp"
#include "plugin_trampoline.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace cerridwen;

std::string get_wasm_path() {
    if (FILE* f = fopen("plugin.wasm", "rb")) {
        fclose(f);
        return "plugin.wasm";
    } else if (FILE* f2 = fopen("tests/plugin.wasm", "rb")) {
        fclose(f2);
        return "tests/plugin.wasm";
    }
    return "build/tests/plugin.wasm";
}

TEST(CerridwenE2E, BasicWasmExecution) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    bind_LoggerHost(instance->get_module()); 
    
    uint32_t wasm_ptr = instance->call_create("MathPlugin_create");
    ASSERT_NE(wasm_ptr, 0);
    
    MathPlugin_Trampoline trampoline;
    trampoline._wasm_instance = instance.get();
    trampoline._wasm_ptr = wasm_ptr;
    
    trampoline.sync_counter();
    EXPECT_EQ(trampoline.counter, 0);
    
    trampoline.increment();
    EXPECT_EQ(trampoline.counter, 1);
    
    trampoline.increment();
    EXPECT_EQ(trampoline.counter, 2);
    
    trampoline.decrement();
    EXPECT_EQ(trampoline.counter, 1);
    
    int product = trampoline.multiply(5, 4);
    EXPECT_EQ(product, 20);
    
    LoggerHost my_logger;
    uint64_t host_ptr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&my_logger));
    
    trampoline.do_logging(host_ptr);
    EXPECT_EQ(trampoline.counter, 123456789);
}

TEST(CerridwenE2E, SingleInheritance) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    uint32_t ptr = instance->call_create("Square_create");
    ASSERT_NE(ptr, 0);
    
    Square_Trampoline sq;
    sq._wasm_instance = instance.get();
    sq._wasm_ptr = ptr;
    
    sq.sync_w();
    sq.sync_h();
    EXPECT_EQ(sq.w, 5);
    EXPECT_EQ(sq.h, 5);
    EXPECT_EQ(sq.sides(), 4);
    EXPECT_EQ(sq.area(), 25);
    EXPECT_EQ(sq.describe(), 4026);
}

TEST(CerridwenE2E, MultipleInheritance) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    uint32_t ptr = instance->call_create("Duck_create");
    ASSERT_NE(ptr, 0);
    
    Duck_Trampoline duck;
    duck._wasm_instance = instance.get();
    duck._wasm_ptr = ptr;
    
    EXPECT_EQ(duck.walk(10), 11);
    EXPECT_EQ(duck.swim(5), 15);
    EXPECT_EQ(duck.fly(), 99);
}

TEST(CerridwenE2E, VirtualInheritance) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    uint32_t ptr = instance->call_create("Widget_create");
    ASSERT_NE(ptr, 0);
    
    Widget_Trampoline widget;
    widget._wasm_instance = instance.get();
    widget._wasm_ptr = ptr;
    
    EXPECT_EQ(widget.total(), 30);
    EXPECT_EQ(widget.kind(), 20); 
    EXPECT_EQ(widget.id(), 2);    
}

TEST(CerridwenE2E, SubclassingHostClass) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    bind_Animal(instance->get_module());

    uint32_t ptr = instance->call_create("Bird_create");
    ASSERT_NE(ptr, 0);
    
    Bird_Trampoline bird;
    bird._wasm_instance = instance.get();
    bird._wasm_ptr = ptr;
    
    Animal my_animal;
    uint64_t host_ptr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&my_animal));
    bird.attach(host_ptr);
    
    EXPECT_EQ(bird.sound_id(), 2);
    EXPECT_EQ(bird.describe(), 22);
}

TEST(CerridwenE2E, ExceptionPropagation) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    bind_Calculator(instance->get_module());

    uint32_t ptr = instance->call_create("Thrower_create");
    ASSERT_NE(ptr, 0);
    
    Thrower_Trampoline thrower;
    thrower._wasm_instance = instance.get();
    thrower._wasm_ptr = ptr;
    
    EXPECT_THROW({ thrower.risky(-1); }, PluginException);
    
    Calculator calc;
    uint64_t host_ptr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&calc));
    
    EXPECT_THROW({ thrower.divide_via_host(host_ptr, 10, 0); }, HostException);
    
    EXPECT_THROW({ thrower.crash(); }, PluginTrap);
}

TEST(CerridwenE2E, MemoryLimitsAndGrowth) {
    auto initial_instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    int initial_pages = initial_instance->memory_bytes() / 65536;
    initial_instance.reset(); // destroy it

    InstanceOptions opts;
    opts.max_memory_bytes = (initial_pages + 2) * 65536;
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path(), opts);
    
    uint32_t ptr = instance->call_create("MemoryProbe_create");
    ASSERT_NE(ptr, 0);
    
    MemoryProbe_Trampoline probe;
    probe._wasm_instance = instance.get();
    probe._wasm_ptr = ptr;
    
    EXPECT_EQ(probe.alloc_block(1024), 1);
    EXPECT_EQ(probe.block_count(), 1);
    
    // Attempt to grow by 10 pages (which would exceed max limit)
    int res = probe.grow(10);
    EXPECT_EQ(res, -1);
}

TEST(CerridwenE2E, StorageSandbox) {
    auto instance = std::make_unique<Wasm3Instance>(get_wasm_path());
    std::string test_dir = "test_sandbox";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directory(test_dir);
    
    StorageHost storage(test_dir, 1000); // 1000 bytes quota
    instance->set_storage(&storage);
    
    uint32_t ptr = instance->call_create("StorageClient_create");
    ASSERT_NE(ptr, 0);
    
    StorageClient_Trampoline client;
    client._wasm_instance = instance.get();
    client._wasm_ptr = ptr;
    
    EXPECT_EQ(client.save_number(42), 0);
    EXPECT_EQ(client.load_number(), 42);
    
    EXPECT_EQ(client.write_blob(500), 0);
    EXPECT_EQ(client.blob_checksum(), 62286); 
    
    EXPECT_EQ(client.write_blob(2000), StorageHost::QuotaExceeded);
    
    EXPECT_EQ(client.try_write(1, 10), StorageHost::InvalidName); 
    EXPECT_EQ(client.try_write(2, 10), StorageHost::InvalidName);
    EXPECT_EQ(client.try_write(3, 10), 0);
    EXPECT_EQ(client.try_write(4, 10), StorageHost::InvalidName);
    EXPECT_EQ(client.try_write(5, 10), StorageHost::InvalidName);
    
    std::filesystem::remove_all(test_dir);
}
