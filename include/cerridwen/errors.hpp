#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace cerridwen {

// Base of every error Cerridwen raises when crossing the host/plugin boundary.
struct CerridwenError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// An error raised inside the plugin (CERRIDWEN_THROW) that propagated to the host.
struct PluginException : CerridwenError {
    using CerridwenError::CerridwenError;
};

// An exception thrown by host code, invoked from the plugin, that propagated back
// out through the plugin to the host caller. The original C++ type is not
// preserved (WebAssembly has no way to carry it) but the what() message is.
struct HostException : CerridwenError {
    using CerridwenError::CerridwenError;
};

// A WebAssembly trap that was not caused by a Cerridwen-managed exception
// (out-of-bounds access, unreachable, missing import, ...).
struct PluginTrap : CerridwenError {
    using CerridwenError::CerridwenError;
};

// A resource limit imposed by the host (memory, storage) was violated.
struct ResourceLimitError : CerridwenError {
    using CerridwenError::CerridwenError;
};

enum class ErrorKind { None, Host, Plugin };

struct PendingError {
    ErrorKind kind = ErrorKind::None;
    std::string message;
};

// The error being carried across a trap. Wasm3 can only unwind a plugin call with a
// trap, so the generated wrappers record the real error here and the instance that
// observes the trap converts it back into a typed C++ exception.
inline PendingError& pending_error() {
    thread_local PendingError p;
    return p;
}

inline void record_host_error(std::string msg) { pending_error() = {ErrorKind::Host, std::move(msg)}; }
inline void record_plugin_error(std::string msg) { pending_error() = {ErrorKind::Plugin, std::move(msg)}; }

inline PendingError take_pending_error() {
    PendingError out = std::move(pending_error());
    pending_error() = {};
    return out;
}

} // namespace cerridwen
