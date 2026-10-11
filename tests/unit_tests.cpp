#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <cerridwen/annotations.hpp>
#include <cerridwen/errors.hpp>
#include <cerridwen/serializer.hpp>
#include <cerridwen/storage.hpp>
#include <cerridwen/atomic.hpp>
#include <cerridwen/cross_domain_atomic.hpp>
#include <cerridwen/plugin_manager.hpp>
#include <cerridwen/plugin_api.hpp>
#include <cerridwen/wasm3_instance.hpp>

namespace fs = std::filesystem;

// =====================================================================================
// 1. Annotations Tests
// =====================================================================================
TEST(CerridwenAnnotationsTest, AnnotationTypesInstantiable) {
    cerridwen::export_wasm a{};
    cerridwen::marshal_by_value b{};
    cerridwen::marshal_by_ref c{};
    cerridwen::thread_safe d{};
    cerridwen::export_method e{};
    cerridwen::export_host f{};
    cerridwen::storage_class g{};

    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
    SUCCEED();
}

// =====================================================================================
// 2. Serializer Unit Tests
// =====================================================================================
TEST(CerridwenSerializerTest, BufferWriteReadAndUnderflow) {
    cerridwen::Buffer buf;
    uint32_t val1 = 12345;
    uint32_t val2 = 67890;

    buf.write(&val1, sizeof(val1));
    buf.write(&val2, sizeof(val2));

    EXPECT_EQ(buf.data.size(), sizeof(val1) + sizeof(val2));
    EXPECT_EQ(buf.offset, 0);

    uint32_t r1 = 0, r2 = 0;
    buf.read(&r1, sizeof(r1));
    EXPECT_EQ(r1, val1);
    EXPECT_EQ(buf.offset, sizeof(val1));

    buf.read(&r2, sizeof(r2));
    EXPECT_EQ(r2, val2);
    EXPECT_EQ(buf.offset, sizeof(val1) + sizeof(val2));

    // Underflow expectation
    uint32_t r3 = 0;
    EXPECT_THROW(buf.read(&r3, sizeof(r3)), std::runtime_error);
}

TEST(CerridwenSerializerTest, PrimitivesSerialization) {
    cerridwen::Buffer buf;

    int8_t i8 = -42;
    int16_t i16 = -1234;
    int32_t i32 = -123456;
    int64_t i64 = -9876543210LL;
    uint8_t u8 = 200;
    uint16_t u16 = 50000;
    uint32_t u32 = 3000000000U;
    uint64_t u64 = 18000000000000000000ULL;
    float f = 3.14159f;
    double d = 2.718281828459045;
    bool b = true;

    cerridwen::serialize(buf, i8);
    cerridwen::serialize(buf, i16);
    cerridwen::serialize(buf, i32);
    cerridwen::serialize(buf, i64);
    cerridwen::serialize(buf, u8);
    cerridwen::serialize(buf, u16);
    cerridwen::serialize(buf, u32);
    cerridwen::serialize(buf, u64);
    cerridwen::serialize(buf, f);
    cerridwen::serialize(buf, d);
    cerridwen::serialize(buf, b);

    int8_t ri8; int16_t ri16; int32_t ri32; int64_t ri64;
    uint8_t ru8; uint16_t ru16; uint32_t ru32; uint64_t ru64;
    float rf; double rd; bool rb;

    cerridwen::deserialize(buf, ri8);
    cerridwen::deserialize(buf, ri16);
    cerridwen::deserialize(buf, ri32);
    cerridwen::deserialize(buf, ri64);
    cerridwen::deserialize(buf, ru8);
    cerridwen::deserialize(buf, ru16);
    cerridwen::deserialize(buf, ru32);
    cerridwen::deserialize(buf, ru64);
    cerridwen::deserialize(buf, rf);
    cerridwen::deserialize(buf, rd);
    cerridwen::deserialize(buf, rb);

    EXPECT_EQ(ri8, i8);
    EXPECT_EQ(ri16, i16);
    EXPECT_EQ(ri32, i32);
    EXPECT_EQ(ri64, i64);
    EXPECT_EQ(ru8, u8);
    EXPECT_EQ(ru16, u16);
    EXPECT_EQ(ru32, u32);
    EXPECT_EQ(ru64, u64);
    EXPECT_FLOAT_EQ(rf, f);
    EXPECT_DOUBLE_EQ(rd, d);
    EXPECT_EQ(rb, b);
}

TEST(CerridwenSerializerTest, StringSerialization) {
    cerridwen::Buffer buf;

    std::string empty;
    std::string hello = "Hello, WebAssembly and Cerridwen!";
    std::string long_str(2048, 'x');

    cerridwen::serialize(buf, empty);
    cerridwen::serialize(buf, hello);
    cerridwen::serialize(buf, long_str);

    std::string out_empty, out_hello, out_long;
    cerridwen::deserialize(buf, out_empty);
    cerridwen::deserialize(buf, out_hello);
    cerridwen::deserialize(buf, out_long);

    EXPECT_EQ(out_empty, empty);
    EXPECT_EQ(out_hello, hello);
    EXPECT_EQ(out_long, long_str);
}

TEST(CerridwenSerializerTest, ContainersSerialization) {
    cerridwen::Buffer buf;

    std::vector<int> vec = {10, 20, 30, 40};
    std::list<std::string> lst = {"alpha", "beta", "gamma"};
    std::deque<double> deq = {1.1, 2.2, 3.3};
    std::set<int> st = {5, 1, 9, 3};
    std::unordered_set<std::string> ust = {"red", "green", "blue"};
    std::map<std::string, int> mp = {{"one", 1}, {"two", 2}, {"three", 3}};
    std::unordered_map<int, std::string> ump = {{100, "hundred"}, {200, "two hundred"}};

    cerridwen::serialize(buf, vec);
    cerridwen::serialize(buf, lst);
    cerridwen::serialize(buf, deq);
    cerridwen::serialize(buf, st);
    cerridwen::serialize(buf, ust);
    cerridwen::serialize(buf, mp);
    cerridwen::serialize(buf, ump);

    std::vector<int> out_vec;
    std::list<std::string> out_lst;
    std::deque<double> out_deq;
    std::set<int> out_st;
    std::unordered_set<std::string> out_ust;
    std::map<std::string, int> out_mp;
    std::unordered_map<int, std::string> out_ump;

    cerridwen::deserialize(buf, out_vec);
    cerridwen::deserialize(buf, out_lst);
    cerridwen::deserialize(buf, out_deq);
    cerridwen::deserialize(buf, out_st);
    cerridwen::deserialize(buf, out_ust);
    cerridwen::deserialize(buf, out_mp);
    cerridwen::deserialize(buf, out_ump);

    EXPECT_EQ(out_vec, vec);
    EXPECT_EQ(out_lst, lst);
    EXPECT_EQ(out_deq, deq);
    EXPECT_EQ(out_st, st);
    EXPECT_EQ(out_ust, ust);
    EXPECT_EQ(out_mp, mp);
    EXPECT_EQ(out_ump, ump);
}

TEST(CerridwenSerializerTest, VariantSerialization) {
    using TestVariant = std::variant<int, std::string, double>;
    cerridwen::Buffer buf;

    TestVariant v1 = 42;
    TestVariant v2 = std::string("Cerridwen Variant");
    TestVariant v3 = 3.14159;

    cerridwen::serialize(buf, v1);
    cerridwen::serialize(buf, v2);
    cerridwen::serialize(buf, v3);

    TestVariant out1, out2, out3;
    cerridwen::deserialize(buf, out1);
    cerridwen::deserialize(buf, out2);
    cerridwen::deserialize(buf, out3);

    EXPECT_EQ(out1.index(), 0);
    EXPECT_EQ(std::get<int>(out1), 42);

    EXPECT_EQ(out2.index(), 1);
    EXPECT_EQ(std::get<std::string>(out2), "Cerridwen Variant");

    EXPECT_EQ(out3.index(), 2);
    EXPECT_DOUBLE_EQ(std::get<double>(out3), 3.14159);
}

// =====================================================================================
// 3. Storage Host Unit Tests
// =====================================================================================
class StorageHostTest : public ::testing::Test {
protected:
    fs::path test_dir;

    void SetUp() override {
        test_dir = fs::temp_directory_path() / ("cerridwen_storage_test_" + std::to_string(std::rand()));
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }
};

TEST_F(StorageHostTest, ValidNameRules) {
    EXPECT_TRUE(cerridwen::StorageHost::valid_name("file.txt"));
    EXPECT_TRUE(cerridwen::StorageHost::valid_name("sub/dir/file.bin"));
    EXPECT_TRUE(cerridwen::StorageHost::valid_name("a_B-1.2.3"));

    // Invalid names
    EXPECT_FALSE(cerridwen::StorageHost::valid_name(""));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("."));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name(".."));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("../escape.txt"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("sub/../escape.txt"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("/abs/path"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("sub//double_slash"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("invalid*char"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("invalid?char"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("invalid:char"));
    EXPECT_FALSE(cerridwen::StorageHost::valid_name("back\\slash"));

    // Long names
    std::string too_long(256, 'a');
    EXPECT_FALSE(cerridwen::StorageHost::valid_name(too_long));

    // Depth exceeding 8
    std::string deep = "1/2/3/4/5/6/7/8/9";
    EXPECT_FALSE(cerridwen::StorageHost::valid_name(deep));
}

TEST_F(StorageHostTest, BasicWriteReadRemove) {
    cerridwen::StorageHost storage(test_dir, 1024 * 1024); // 1 MB quota

    EXPECT_EQ(storage.quota(), 1024 * 1024);
    EXPECT_FALSE(storage.root().empty());
    EXPECT_EQ(storage.used(), 0);

    const std::string test_data = "Hello Storage Sandbox!";
    EXPECT_EQ(storage.write("hello.txt", test_data.data(), test_data.size()), cerridwen::StorageHost::Ok);
    EXPECT_TRUE(storage.exists("hello.txt"));
    EXPECT_EQ(storage.size("hello.txt"), static_cast<int64_t>(test_data.size()));
    EXPECT_EQ(storage.used(), test_data.size());

    // Read back
    char buffer[64] = {0};
    int64_t bytes_read = storage.read("hello.txt", buffer, sizeof(buffer));
    EXPECT_EQ(bytes_read, static_cast<int64_t>(test_data.size()));
    EXPECT_EQ(std::string(buffer, bytes_read), test_data);

    // Partial read
    char small_buf[5] = {0};
    int64_t partial = storage.read("hello.txt", small_buf, sizeof(small_buf));
    EXPECT_EQ(partial, static_cast<int64_t>(test_data.size()));
    EXPECT_EQ(std::string(small_buf, 5), test_data.substr(0, 5));

    // Remove
    EXPECT_EQ(storage.remove("hello.txt"), cerridwen::StorageHost::Ok);
    EXPECT_FALSE(storage.exists("hello.txt"));
    EXPECT_EQ(storage.used(), 0);

    // Remove non-existent
    EXPECT_EQ(storage.remove("hello.txt"), cerridwen::StorageHost::NotFound);
    EXPECT_EQ(storage.read("hello.txt", buffer, sizeof(buffer)), cerridwen::StorageHost::NotFound);
}

TEST_F(StorageHostTest, QuotaEnforcement) {
    cerridwen::StorageHost storage(test_dir, 100); // 100 bytes quota

    std::string data_50(50, 'A');
    std::string data_60(60, 'B');

    EXPECT_EQ(storage.write("first.bin", data_50.data(), data_50.size()), cerridwen::StorageHost::Ok);
    EXPECT_EQ(storage.used(), 50);

    // 50 + 60 > 100 => QuotaExceeded
    EXPECT_EQ(storage.write("second.bin", data_60.data(), data_60.size()), cerridwen::StorageHost::QuotaExceeded);
    EXPECT_FALSE(storage.exists("second.bin"));

    // Overwriting first.bin with 40 bytes fits within quota
    std::string data_40(40, 'C');
    EXPECT_EQ(storage.write("first.bin", data_40.data(), data_40.size()), cerridwen::StorageHost::Ok);
    EXPECT_EQ(storage.used(), 40);

    // Invalid name write
    EXPECT_EQ(storage.write("../hack.bin", data_40.data(), data_40.size()), cerridwen::StorageHost::InvalidName);
    EXPECT_EQ(storage.read("../hack.bin", nullptr, 0), cerridwen::StorageHost::InvalidName);
    EXPECT_EQ(storage.remove("../hack.bin"), cerridwen::StorageHost::InvalidName);
}

// =====================================================================================
// 4. Errors Unit Tests
// =====================================================================================
TEST(CerridwenErrorsTest, ExceptionHierarchy) {
    try {
        throw cerridwen::PluginException("plugin error");
    } catch (const cerridwen::CerridwenError& e) {
        EXPECT_STREQ(e.what(), "plugin error");
    }

    try {
        throw cerridwen::HostException("host error");
    } catch (const cerridwen::CerridwenError& e) {
        EXPECT_STREQ(e.what(), "host error");
    }

    try {
        throw cerridwen::MemoryAccessException("memory bounds");
    } catch (const cerridwen::PluginTrap& e) {
        EXPECT_STREQ(e.what(), "memory bounds");
    }

    try {
        throw cerridwen::ResourceLimitError("quota exceeded");
    } catch (const cerridwen::CerridwenError& e) {
        EXPECT_STREQ(e.what(), "quota exceeded");
    }
}

TEST(CerridwenErrorsTest, PendingErrorRecordingAndTake) {
    // Clear any previous error
    cerridwen::take_pending_error();

    auto pe = cerridwen::take_pending_error();
    EXPECT_EQ(pe.kind, cerridwen::ErrorKind::None);
    EXPECT_TRUE(pe.message.empty());

    cerridwen::record_host_error("Host failed during operation");
    EXPECT_EQ(cerridwen::pending_error().kind, cerridwen::ErrorKind::Host);
    EXPECT_EQ(cerridwen::pending_error().message, "Host failed during operation");

    pe = cerridwen::take_pending_error();
    EXPECT_EQ(pe.kind, cerridwen::ErrorKind::Host);
    EXPECT_EQ(pe.message, "Host failed during operation");

    // Once taken, pending_error is cleared
    EXPECT_EQ(cerridwen::pending_error().kind, cerridwen::ErrorKind::None);

    cerridwen::record_plugin_error("Plugin fatal error");
    pe = cerridwen::take_pending_error();
    EXPECT_EQ(pe.kind, cerridwen::ErrorKind::Plugin);
    EXPECT_EQ(pe.message, "Plugin fatal error");
}

// =====================================================================================
// 5. Plugin Manager & WasmInstance Base Unit Tests
// =====================================================================================
TEST(CerridwenPluginManagerTest, ShareToWasmHandle) {
    EXPECT_EQ(cerridwen::share_to_wasm<int>(nullptr), 0);

    auto sp = std::make_shared<int>(42);
    uint64_t handle = cerridwen::share_to_wasm(sp);
    EXPECT_NE(handle, 0);

    auto* restored = reinterpret_cast<std::shared_ptr<int>*>(static_cast<uintptr_t>(handle));
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(**restored, 42);
    delete restored;
}

TEST(CerridwenPluginManagerTest, WasmInstanceVirtualFallbacks) {
    cerridwen::WasmInstance inst;
    EXPECT_THROW(inst.call("fn", 0), std::runtime_error);
    EXPECT_THROW(inst.call_int("fn", 0), std::runtime_error);
    EXPECT_THROW(inst.call_set_int("fn", 0, 1), std::runtime_error);
    EXPECT_THROW(inst.read_memory(0, nullptr, 0), std::runtime_error);
    EXPECT_THROW(inst.write_memory(0, nullptr, 0), std::runtime_error);
    EXPECT_THROW(inst.get_memory_ptr(0), std::runtime_error);
    EXPECT_THROW(inst.allocate(10), std::runtime_error);
    EXPECT_THROW(inst.deallocate(0), std::runtime_error);
    EXPECT_THROW(inst.memory_bytes(), std::runtime_error);
    EXPECT_EQ(inst.memory_limit(), 0);
    EXPECT_THROW(inst.call_args("fn", {}), std::runtime_error);
    EXPECT_THROW(inst.call_args_int("fn", {}), std::runtime_error);
    EXPECT_EQ(inst.instantiate<int>("Dummy"), nullptr);
}

TEST(CerridwenPluginManagerTest, PluginManagerLoadsMock) {
    cerridwen::PluginManager pm;
    auto inst = pm.load_plugin("test_mock.wasm");
    ASSERT_NE(inst, nullptr);
}

// =====================================================================================
// 6. Cross Domain Atomic Unit Tests
// =====================================================================================
TEST(CerridwenAtomicTest, NativeCrossDomainAtomicOperations) {
    cerridwen::CrossDomainAtomic<int> atom(10);
    EXPECT_EQ(atom.load(), 10);

    atom.store(25);
    EXPECT_EQ(atom.load(), 25);

    EXPECT_EQ(atom.exchange(50), 25);
    EXPECT_EQ(atom.load(), 50);

    EXPECT_EQ(atom.fetch_add(5), 50);
    EXPECT_EQ(atom.load(), 55);

    EXPECT_EQ(atom.fetch_sub(15), 55);
    EXPECT_EQ(atom.load(), 40);

    int expected = 40;
    EXPECT_TRUE(atom.compare_exchange_strong(expected, 100));
    EXPECT_EQ(atom.load(), 100);

    expected = 40; // mismatch
    EXPECT_FALSE(atom.compare_exchange_strong(expected, 200));
    EXPECT_EQ(expected, 100);
    EXPECT_EQ(atom.load(), 100);

    EXPECT_NE(atom.ptr(), nullptr);
}

// Mock WasmInstance for testing HostCrossDomainAtomic
class MockAtomicWasmInstance : public cerridwen::WasmInstance {
    int memory_cell = 0;
public:
    void* get_memory_ptr(uint32_t wasm_ptr) override {
        (void)wasm_ptr;
        return &memory_cell;
    }
};

TEST(CerridwenAtomicTest, HostCrossDomainAtomicOperations) {
    MockAtomicWasmInstance mock_inst;
    cerridwen::HostCrossDomainAtomic<int> host_atom(&mock_inst, 0);

    host_atom.store(77);
    EXPECT_EQ(host_atom.load(), 77);

    EXPECT_EQ(host_atom.fetch_add(10), 77);
    EXPECT_EQ(host_atom.load(), 87);

    EXPECT_EQ(host_atom.fetch_sub(7), 87);
    EXPECT_EQ(host_atom.load(), 80);

    EXPECT_EQ(host_atom.exchange(200), 80);
    EXPECT_EQ(host_atom.load(), 200);

    int expected = 200;
    EXPECT_TRUE(host_atom.compare_exchange_strong(expected, 999));
    EXPECT_EQ(host_atom.load(), 999);
}

// =====================================================================================
// 7. Plugin API Native Fallbacks
// =====================================================================================
TEST(CerridwenPluginApiTest, NativeAllocAndRelease) {
    void* mem = cerridwen::plugin::allocate(128);
    ASSERT_NE(mem, nullptr);
    std::memset(mem, 0xAB, 128);
    cerridwen::plugin::release(mem);

    // CERRIDWEN_THROW in native mode raises PluginException
    EXPECT_THROW(CERRIDWEN_THROW("native test throw"), cerridwen::PluginException);
}

// =====================================================================================
// 8. Wasm3Instance Unit Tests
// =====================================================================================
TEST(CerridwenWasm3InstanceTest, NonExistentFileThrows) {
    EXPECT_THROW(cerridwen::Wasm3Instance("non_existent_file_12345.wasm"), std::runtime_error);
}

TEST(CerridwenWasm3InstanceTest, StorageHostAttachment) {
    std::string wasm_path = "build/tests/plugin.wasm";
    if (!fs::exists(wasm_path)) wasm_path = "tests/plugin.wasm";
    if (!fs::exists(wasm_path)) wasm_path = "plugin.wasm";

    if (fs::exists(wasm_path)) {
        cerridwen::Wasm3Instance instance(wasm_path);
        EXPECT_NE(instance.get_memory(), nullptr);
        EXPECT_GT(instance.memory_bytes(), 0);

        fs::path tmp_root = fs::temp_directory_path() / "wasm_storage_unit_test";
        cerridwen::StorageHost storage(tmp_root, 65536);
        instance.set_storage(&storage);
        instance.set_storage(nullptr);
        std::error_code ec;
        fs::remove_all(tmp_root, ec);
    }
}

TEST(CerridwenWasm3InstanceTest, ExceedInitialMemoryLimitThrows) {
    std::string wasm_path = "build/tests/plugin.wasm";
    if (!fs::exists(wasm_path)) wasm_path = "tests/plugin.wasm";
    if (!fs::exists(wasm_path)) wasm_path = "plugin.wasm";

    if (fs::exists(wasm_path)) {
        cerridwen::InstanceOptions opts;
        opts.max_memory_bytes = 1024; // smaller than 64KB initial page
        EXPECT_THROW(cerridwen::Wasm3Instance(wasm_path, opts), cerridwen::ResourceLimitError);
    }
}

TEST(CerridwenWasm3InstanceTest, MemoryBoundsChecks) {
    std::string wasm_path = "build/tests/plugin.wasm";
    if (!fs::exists(wasm_path)) wasm_path = "tests/plugin.wasm";
    if (!fs::exists(wasm_path)) wasm_path = "plugin.wasm";

    if (fs::exists(wasm_path)) {
        cerridwen::Wasm3Instance instance(wasm_path);
        uint32_t out_of_bounds_ptr = 0xFFFFFF00;
        char buf[16];
        EXPECT_THROW(instance.read_memory(out_of_bounds_ptr, buf, sizeof(buf)), std::runtime_error);
        EXPECT_THROW(instance.write_memory(out_of_bounds_ptr, buf, sizeof(buf)), std::runtime_error);
        EXPECT_THROW(instance.get_memory_ptr(out_of_bounds_ptr), std::runtime_error);

        uint32_t valid_ptr = instance.allocate(32);
        EXPECT_NE(valid_ptr, 0);
        EXPECT_NO_THROW(instance.write_memory(valid_ptr, "test_data", 9));
        char read_buf[16] = {0};
        EXPECT_NO_THROW(instance.read_memory(valid_ptr, read_buf, 9));
        EXPECT_STREQ(read_buf, "test_data");
        EXPECT_NO_THROW(instance.deallocate(valid_ptr));
    }
}
