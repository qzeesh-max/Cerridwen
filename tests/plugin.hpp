#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <cerridwen/annotations.hpp>
#include <cerridwen/plugin_api.hpp>
#include "host_classes.hpp"

#ifdef __EMSCRIPTEN__
#include "plugin_host_proxies.hpp"
#endif

// =====================================================================================
// Basic plugin: counter state (marshalled by value), upcall into a host class.
// =====================================================================================
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
inline void MathPlugin::do_logging(uint64_t logger_ptr) {
    LoggerHost_Proxy logger(logger_ptr);
    logger.log_message("Hello from WASM Plugin!");
    int ts = logger.get_timestamp();
    this->counter = ts; // store it to verify
}
#else
inline void MathPlugin::do_logging(uint64_t) {}
#endif

// =====================================================================================
// Single inheritance (abstract base -> concrete -> concrete) with virtual dispatch.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM Shape {
    virtual ~Shape() = default;
    virtual int sides() = 0;
    virtual int area() = 0;
    virtual int describe() { return sides() * 1000 + area(); }
};

struct CERRIDWEN_EXPORT_WASM Rect : Shape {
    CERRIDWEN_MARSHAL_BY_VALUE
    int w = 3;
    CERRIDWEN_MARSHAL_BY_VALUE
    int h = 4;

    int sides() override { return 4; }
    int area() override { return w * h; }
};

struct CERRIDWEN_EXPORT_WASM Square : Rect {
    Square() { w = 5; h = 5; }
    int describe() override { return Rect::describe() + 1; }
};

// =====================================================================================
// Multiple inheritance.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM Walker {
    virtual ~Walker() = default;
    virtual int walk(int steps) { return steps; }
};

struct CERRIDWEN_EXPORT_WASM Swimmer {
    virtual ~Swimmer() = default;
    virtual int swim(int laps) { return laps * 2; }
};

struct CERRIDWEN_EXPORT_WASM Duck : Walker, Swimmer {
    int walk(int steps) override { return steps + 1; }
    int swim(int laps) override { return laps * 3; }
    virtual int fly() { return 99; }
};

// =====================================================================================
// Virtual (diamond) inheritance.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM Entity {
    virtual ~Entity() = default;
    virtual int id() { return 1; }
    virtual int kind() { return 10; }
};

struct CERRIDWEN_EXPORT_WASM Named : virtual Entity {
    int kind() override { return 20; }
    virtual int name_len() { return 5; }
};

struct CERRIDWEN_EXPORT_WASM Tagged : virtual Entity {
    int id() override { return 2; }
    virtual int tag_count() { return 3; }
};

struct CERRIDWEN_EXPORT_WASM Widget : Named, Tagged {
    virtual int total() { return id() + kind() + name_len() + tag_count(); }
};

// =====================================================================================
// Plugin class that SUBCLASSES a host class. Inside the plugin the base is a proxy that
// forwards to a host object (bound via attach()), so `AnimalBase::sound_id()` is a
// "super call" executed by the host. On the host side the base is the real Animal.
// =====================================================================================
#ifdef __EMSCRIPTEN__
using AnimalBase = Animal_Proxy;
#else
using AnimalBase = Animal;
#endif

struct CERRIDWEN_EXPORT_WASM Bird : AnimalBase {
    int legs() override { return 2; }
    int sound_id() override { return AnimalBase::sound_id() + 1; }
    int describe() override { return legs() * 10 + sound_id(); }

    virtual void attach(uint64_t host_object) {
#ifdef __EMSCRIPTEN__
        this->cerridwen_bind(host_object);
#else
        (void)host_object;
#endif
    }
};

// =====================================================================================
// Exceptions: plugin -> host, and host -> plugin -> host.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM Thrower {
    virtual ~Thrower() = default;

    // Raises a plugin error when x < 0.
    virtual int risky(int x) {
        if (x < 0) CERRIDWEN_THROW("negative input");
        return x * 2;
    }

    // Calls Calculator::divide on the host; a host exception propagates out.
    virtual int divide_via_host(uint64_t calc, int a, int b);
    // Calls Calculator::add on the host (virtual dispatch into host subclasses).
    virtual int add_via_host(uint64_t calc, int a, int b);

    // Uses a SharedProxy handle
    virtual int divide_via_shared_host(uint64_t calc_handle, int a, int b);

    // A genuine WebAssembly trap that is not a managed exception.
    virtual void crash();
};

#ifdef __EMSCRIPTEN__
inline int Thrower::divide_via_host(uint64_t calc, int a, int b) { Calculator_Proxy c(calc); return c.divide(a, b); }
inline int Thrower::add_via_host(uint64_t calc, int a, int b) { Calculator_Proxy c(calc); return c.add(a, b); }
inline int Thrower::divide_via_shared_host(uint64_t calc_handle, int a, int b) { Calculator_SharedProxy c(calc_handle); return c.divide(a, b); }
inline void Thrower::crash() { __builtin_trap(); }
#else
inline int Thrower::divide_via_host(uint64_t, int, int) { return 0; }
inline int Thrower::add_via_host(uint64_t, int, int) { return 0; }
inline int Thrower::divide_via_shared_host(uint64_t, int, int) { return 0; }
inline void Thrower::crash() {}
#endif

// =====================================================================================
// Memory: allocation, growth, and limits.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM MemoryProbe {
    virtual ~MemoryProbe() { free_all(); }

    // Allocates `bytes` on the plugin heap and touches them. Returns 1 on success, 0 if refused.
    virtual int alloc_block(int bytes) {
        if (count_ >= kMaxBlocks) return 0;
        void* p = cerridwen::plugin::allocate(static_cast<size_t>(bytes));
        if (!p) return 0;
        std::memset(p, 0xAB, static_cast<size_t>(bytes));
        blocks_[count_++] = p;
        return 1;
    }

    virtual int free_all() {
        int n = count_;
        for (int i = 0; i < count_; ++i) cerridwen::plugin::release(blocks_[i]);
        count_ = 0;
        return n;
    }

    virtual int block_count() { return count_; }

#ifdef __EMSCRIPTEN__
    virtual int pages() { return cerridwen::plugin::memory_pages(); }
    virtual int grow(int pages_to_add) { return cerridwen::plugin::grow_memory(pages_to_add); }
#else
    virtual int pages() { return 0; }
    virtual int grow(int) { return -1; }
#endif

private:
    static constexpr int kMaxBlocks = 64;
    void* blocks_[kMaxBlocks] = {};
    int count_ = 0;
};

// =====================================================================================
// Storage: the plugin persists data through the host's sandboxed, quota-limited store.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM StorageClient {
    virtual ~StorageClient() = default;

    virtual int save_number(int value);
    virtual int load_number();            // -2147483647 on failure
    virtual int write_blob(int len);      // blob.bin = bytes (i & 0xFF); returns result code (0 ok)
    virtual int blob_checksum();          // sum of blob.bin bytes, or -1
    virtual int try_write(int which, int len); // jail-escape / name attempts, returns result code
    virtual int remove_blob();
    virtual int used_bytes();
};

#ifdef __EMSCRIPTEN__
inline int StorageClient::save_number(int value) {
    char buf[16];
    int n = 0;
    unsigned v = value < 0 ? 0u - static_cast<unsigned>(value) : static_cast<unsigned>(value);
    char tmp[16]; int t = 0;
    do { tmp[t++] = static_cast<char>('0' + v % 10); v /= 10; } while (v);
    if (value < 0) buf[n++] = '-';
    while (t) buf[n++] = tmp[--t];
    return cerridwen::plugin::Storage().write("number.txt", buf, static_cast<uint32_t>(n));
}

inline int StorageClient::load_number() {
    char buf[16] = {};
    int n = cerridwen::plugin::Storage().read("number.txt", buf, sizeof(buf) - 1);
    if (n < 0) return -2147483647;
    int i = 0, sign = 1, v = 0;
    if (buf[0] == '-') { sign = -1; i = 1; }
    for (; i < n && buf[i] >= '0' && buf[i] <= '9'; ++i) v = v * 10 + (buf[i] - '0');
    return sign * v;
}

inline int StorageClient::write_blob(int len) {
    unsigned char* data = static_cast<unsigned char*>(cerridwen::plugin::allocate(static_cast<size_t>(len)));
    if (!data) return -3;
    for (int i = 0; i < len; ++i) data[i] = static_cast<unsigned char>(i & 0xFF);
    int r = cerridwen::plugin::Storage().write("blob.bin", data, static_cast<uint32_t>(len));
    cerridwen::plugin::release(data);
    return r;
}

inline int StorageClient::blob_checksum() {
    cerridwen::plugin::Storage st;
    int size = st.size("blob.bin");
    if (size < 0) return -1;
    unsigned char* data = static_cast<unsigned char*>(cerridwen::plugin::allocate(static_cast<size_t>(size) + 1));
    if (!data) return -1;
    int n = st.read("blob.bin", data, static_cast<uint32_t>(size));
    int sum = 0;
    for (int i = 0; i < n; ++i) sum += data[i];
    cerridwen::plugin::release(data);
    return sum;
}

inline int StorageClient::try_write(int which, int len) {
    static const char* const names[] = {"ok.bin", "../escape.bin", "/abs.bin", "sub/dir/a.bin", "back\\slash", "a/../../b.bin"};
    if (which < 0 || which >= static_cast<int>(sizeof(names) / sizeof(names[0]))) return -1;
    unsigned char* data = static_cast<unsigned char*>(cerridwen::plugin::allocate(static_cast<size_t>(len) + 1));
    if (!data) return -3;
    std::memset(data, 0x5A, static_cast<size_t>(len));
    int r = cerridwen::plugin::Storage().write(names[which], data, static_cast<uint32_t>(len));
    cerridwen::plugin::release(data);
    return r;
}

inline int StorageClient::remove_blob() { return cerridwen::plugin::Storage().remove("blob.bin"); }
inline int StorageClient::used_bytes() { return static_cast<int>(cerridwen::plugin::Storage().used()); }
#else
inline int StorageClient::save_number(int) { return 0; }
inline int StorageClient::load_number() { return 0; }
inline int StorageClient::write_blob(int) { return 0; }
inline int StorageClient::blob_checksum() { return 0; }
inline int StorageClient::try_write(int, int) { return 0; }
inline int StorageClient::remove_blob() { return 0; }
inline int StorageClient::used_bytes() { return 0; }
#endif

// =====================================================================================
// Container marshaling: vector and map passed by reference.
// =====================================================================================
struct CERRIDWEN_EXPORT_WASM ContainerPlugin {
    virtual ~ContainerPlugin() = default;

    virtual void process_containers(std::vector<int>& vec, std::map<int, int>& m);
    virtual void process_other_containers(std::set<int>& s, std::list<int>& l, std::deque<int>& d, std::unordered_set<int>& us, std::unordered_map<int, int>& um);
};

#ifdef __EMSCRIPTEN__
inline void ContainerPlugin::process_containers(std::vector<int>& vec, std::map<int, int>& m) {
    for (int& v : vec) v += 10;
    vec.push_back(999);
    for (auto& pair : m) {
        pair.second += 10;
    }
    m[100] = 200;
}

inline void ContainerPlugin::process_other_containers(std::set<int>& s, std::list<int>& l, std::deque<int>& d, std::unordered_set<int>& us, std::unordered_map<int, int>& um) {
    std::set<int> new_s;
    for (auto v : s) new_s.insert(v + 10);
    new_s.insert(999);
    s = std::move(new_s);

    for (auto it = l.begin(); it != l.end(); ++it) *it += 10;
    l.push_back(999);

    for (auto it = d.begin(); it != d.end(); ++it) *it += 10;
    d.push_back(999);

    std::unordered_set<int> new_us;
    for (auto v : us) new_us.insert(v + 10);
    new_us.insert(999);
    us = std::move(new_us);

    for (auto& pair : um) {
        pair.second += 10;
    }
    um[100] = 200;
}
#else
inline void ContainerPlugin::process_containers(std::vector<int>&, std::map<int, int>&) {}
inline void ContainerPlugin::process_other_containers(std::set<int>&, std::list<int>&, std::deque<int>&, std::unordered_set<int>&, std::unordered_map<int, int>&) {}
#endif
