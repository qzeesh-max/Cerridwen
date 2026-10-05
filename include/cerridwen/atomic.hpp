#pragma once

#include <atomic>
#include <cstdint>
#include "annotations.hpp"

namespace cerridwen {

// Cross-domain atomic that can be used safely by both Host and Plugin.
// In the plugin, it maps to a standard std::atomic.
// On the host, the framework intercepts accesses to the underlying linear memory if marshalled by reference.
// Note: Requires that the memory location remains valid.
#ifdef __EMSCRIPTEN__

// Plugin implementation of CrossDomainAtomic.
template <typename T>
class CrossDomainAtomic {
public:
    CrossDomainAtomic() noexcept : _val(T{}) {}
    explicit CrossDomainAtomic(T val) noexcept : _val(val) {}

    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept { return _val.load(order); }
    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept { _val.store(desired, order); }
    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept { return _val.exchange(desired, order); }
    bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept { return _val.compare_exchange_weak(expected, desired, success, failure); }
    bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept { return _val.compare_exchange_strong(expected, desired, success, failure); }
    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept { return _val.fetch_add(arg, order); }
    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept { return _val.fetch_sub(arg, order); }
private:
    std::atomic<T> _val;
};

#else

#include "wasm3_instance.hpp"

// Host implementation of CrossDomainAtomic.
// This acts as a proxy to the atomic value inside WASM memory.
template <typename T>
class CrossDomainAtomic {
public:
    CrossDomainAtomic(Wasm3Instance* instance, uint32_t wasm_ptr)
        : _instance(instance), _wasm_ptr(wasm_ptr) {}

    std::atomic<T>* get_atomic() const {
        return reinterpret_cast<std::atomic<T>*>(_instance->get_memory() + _wasm_ptr);
    }

    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept { return get_atomic()->load(order); }
    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept { get_atomic()->store(desired, order); }
    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept { return get_atomic()->exchange(desired, order); }
    bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept { return get_atomic()->compare_exchange_weak(expected, desired, success, failure); }
    bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept { return get_atomic()->compare_exchange_strong(expected, desired, success, failure); }
    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept { return get_atomic()->fetch_add(arg, order); }
    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept { return get_atomic()->fetch_sub(arg, order); }

// Make them public for generator access
public:
    Wasm3Instance* _instance;
    uint32_t _wasm_ptr;
};

#endif

using AtomicInt32 = CrossDomainAtomic<int32_t>;
using AtomicInt64 = CrossDomainAtomic<int64_t>;
using AtomicUint32 = CrossDomainAtomic<uint32_t>;
using AtomicUint64 = CrossDomainAtomic<uint64_t>;

} // namespace cerridwen
