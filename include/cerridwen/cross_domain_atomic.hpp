#pragma once

#ifdef __EMSCRIPTEN__
#include <atomic>

namespace cerridwen {

template<typename T>
class CrossDomainAtomic {
    std::atomic<T> value;
public:
    CrossDomainAtomic(T initial = T()) : value(initial) {}

    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return value.load(order);
    }

    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        value.store(desired, order);
    }

    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.fetch_add(arg, order);
    }

    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.fetch_sub(arg, order);
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.compare_exchange_strong(expected, desired, order);
    }

    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.exchange(desired, order);
    }

    T* ptr() noexcept { return reinterpret_cast<T*>(&value); }
};

} // namespace cerridwen

#else

#include <atomic>
#include <cerridwen/plugin_manager.hpp>

namespace cerridwen {

template<typename T>
class CrossDomainAtomic {
    std::atomic<T> value;
public:
    CrossDomainAtomic(T initial = T()) : value(initial) {}

    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return value.load(order);
    }

    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        value.store(desired, order);
    }

    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.fetch_add(arg, order);
    }

    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.fetch_sub(arg, order);
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.compare_exchange_strong(expected, desired, order);
    }

    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value.exchange(desired, order);
    }

    T* ptr() noexcept { return reinterpret_cast<T*>(&value); }
};

template<typename T>
class HostCrossDomainAtomic {
    std::atomic_ref<T> atomic_ref;

public:
    HostCrossDomainAtomic(WasmInstance* instance, uint32_t wasm_ptr) 
        : atomic_ref(*reinterpret_cast<T*>(instance->get_memory_ptr(wasm_ptr))) {}

    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return atomic_ref.load(order);
    }

    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        atomic_ref.store(desired, order);
    }

    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return atomic_ref.fetch_add(arg, order);
    }

    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return atomic_ref.fetch_sub(arg, order);
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return atomic_ref.compare_exchange_strong(expected, desired, order);
    }

    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return atomic_ref.exchange(desired, order);
    }
};

} // namespace cerridwen

#endif
