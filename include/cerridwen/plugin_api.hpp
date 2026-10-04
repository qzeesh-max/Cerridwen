#pragma once

// Plugin-side runtime API. Include this from code that is compiled to WebAssembly
// (it is also safe to include in a native build, where CERRIDWEN_THROW degrades to a
// real C++ throw so plugin logic can be unit-tested natively).

#include <cerridwen/annotations.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#ifdef __EMSCRIPTEN__

extern "C" {
__attribute__((import_module("env"), import_name("cerridwen_throw")))
void cerridwen_throw(const char* message);

__attribute__((import_module("env"), import_name("cerridwen_storage_write")))
int cerridwen_storage_write(const char* name, const void* data, uint32_t len);
__attribute__((import_module("env"), import_name("cerridwen_storage_read")))
int cerridwen_storage_read(const char* name, void* buf, uint32_t cap);
__attribute__((import_module("env"), import_name("cerridwen_storage_size")))
int cerridwen_storage_size(const char* name);
__attribute__((import_module("env"), import_name("cerridwen_storage_remove")))
int cerridwen_storage_remove(const char* name);
__attribute__((import_module("env"), import_name("cerridwen_storage_used")))
uint64_t cerridwen_storage_used();
__attribute__((import_module("env"), import_name("cerridwen_storage_quota")))
uint64_t cerridwen_storage_quota();
}

namespace cerridwen::plugin {

// Raise an error to the host. Unwinds the plugin call (via a trap) and surfaces on the
// host as cerridwen::PluginException carrying `message`.
[[noreturn]] inline void throw_error(const char* message) {
    cerridwen_throw(message);
    __builtin_trap();
}

// ---- Memory -------------------------------------------------------------------------

// Allocates from the plugin's own heap. Returns nullptr if the heap cannot grow
// (plugin-declared maximum or host-imposed limit reached).
inline void* allocate(size_t bytes) { return std::malloc(bytes); }
inline void release(void* p) { std::free(p); }

// Linear memory size in 64KiB pages.
inline int memory_pages() { return static_cast<int>(__builtin_wasm_memory_size(0)); }

// Explicitly grows linear memory by `pages`. Returns the previous size in pages, or
// -1 if the growth was refused (maximum or host limit).
inline int grow_memory(int pages) { return static_cast<int>(__builtin_wasm_memory_grow(0, static_cast<size_t>(pages))); }

// ---- Storage ------------------------------------------------------------------------

// Handle to the host-provided, sandboxed, quota-limited storage area. All methods
// return a non-negative value on success or a negative StorageHost::Result code:
//   -1 invalid name, -2 quota exceeded, -3 I/O error, -4 not found, -5 no storage.
class Storage {
public:
    int write(const char* name, const void* data, uint32_t len) const { return cerridwen_storage_write(name, data, len); }
    // Returns total blob size; copies at most `cap` bytes.
    int read(const char* name, void* buf, uint32_t cap) const { return cerridwen_storage_read(name, buf, cap); }
    int size(const char* name) const { return cerridwen_storage_size(name); }
    bool exists(const char* name) const { return cerridwen_storage_size(name) >= 0; }
    int remove(const char* name) const { return cerridwen_storage_remove(name); }
    uint64_t used() const { return cerridwen_storage_used(); }
    uint64_t quota() const { return cerridwen_storage_quota(); }
};

} // namespace cerridwen::plugin

#define CERRIDWEN_THROW(msg) ::cerridwen::plugin::throw_error(msg)

#else // native build

#include <cerridwen/errors.hpp>
#define CERRIDWEN_THROW(msg) throw ::cerridwen::PluginException(msg)

namespace cerridwen::plugin {
inline void* allocate(size_t bytes) { return std::malloc(bytes); }
inline void release(void* p) { std::free(p); }
} // namespace cerridwen::plugin

#endif
