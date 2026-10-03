# Cerridwen

<p align="center">
  <img src="assets/cerridwen_logo.jpg" alt="Cerridwen Framework Logo" width="300"/>
</p>

Cerridwen is a modern, high-performance C++26 framework designed to securely expose C++ classes to WebAssembly (WASM) and allow WebAssembly plugins to be loaded seamlessly within native applications. By leveraging C++26 reflection (`std::meta`), Cerridwen automatically generates bindings, trampolines, and proxy classes without requiring external AST parsers or boilerplate code.

## The Mythos

In Celtic mythology, **Cerridwen** is a powerful enchantress and the keeper of the cauldron of knowledge, inspiration, and transformation (the *Awen*). The potion brewed within her cauldron grants profound wisdom and the ability to shape-shift into any form.

We chose this name because the framework acts as a modern cauldron of transformation for C++ applications. By brewing C++ code with WebAssembly, Cerridwen transforms static, monolithic host applications into dynamic, extensible platforms. It securely confines untrusted WebAssembly plugins while granting them the "wisdom" to interoperate seamlessly with complex C++ class hierarchies, seamlessly shifting shapes across the WebAssembly/Native boundary. 

## Key Features

* **C++26 Reflection (`std::meta`)**: Automatically analyzes your C++ class structures at compile time to generate bridging code for WebAssembly, eliminating the need for SWIG or manual binding layers.
* **Full Inheritance Support**: Natively supports Single Inheritance, Multiple Inheritance, and Virtual Inheritance across the WebAssembly boundary. The framework automatically adjusts `this` pointers when dispatching calls from WASM plugins back to the host C++ objects.
* **Virtual Method Overrides (Trampolines)**: WebAssembly plugins can subclass C++ classes and override virtual methods. When the C++ host calls a virtual method, Cerridwen's generated trampolines dynamically route the execution into the WASM sandbox and marshal the return values back.
* **Proxy-Based Data Marshalling**: Complex C++ data members (strings, vectors, custom structs) are safely marshaled between the host and plugins using Reference Proxies and memory offset mapping, ensuring minimal allocations and robust memory bounds checking.
* **Multi-Threading & Concurrency Gating**: Since WebAssembly instances are inherently single-threaded, Cerridwen provides multi-threading models (`[[=cerridwen::thread_safe{}]]`) to safely gate, lock, and serialize access when multiple native C++ threads invoke WebAssembly plugins concurrently.
* **Test-Driven Design**: Built with rigorous TDD principles to guarantee absolute memory safety when bridging native C++ with sandboxed plugins.

## ⚡ Quick Start

Cerridwen relies heavily on C++26 structural annotations to guide the compile-time generation of WebAssembly bindings.

```cpp
#include <cerridwen/exporter.hpp>
#include <cerridwen/annotations.hpp>
#include <string>
#include <iostream>

// 1. Export the C++ Interface to WebAssembly
struct [[=cerridwen::export_wasm{}]] PluginInterface {
    virtual ~PluginInterface() = default;

    // A virtual method that the WebAssembly plugin will implement
    virtual void process_data(int input) = 0;
    
    // A concrete method the WASM plugin can call on the host
    void log_message(const std::string& msg) {
        std::cout << "[Host] Plugin logged: " << msg << "\n";
    }
};

// 2. Generate the WASM Bindings
CERRIDWEN_EXPORT_CLASS(PluginInterface);
```

To load and interact with the plugin from your host application:

```cpp
#include <cerridwen/plugin_manager.hpp>

int main() {
    cerridwen::PluginManager manager;
    
    // Load the WebAssembly module
    auto plugin = manager.load_plugin("my_plugin.wasm");
    
    // Instantiate the exported C++ class implementation from the plugin
    auto instance = plugin->instantiate<PluginInterface>("MyWasmPluginClass");
    
    // Call the virtual method. The execution securely drops into the WebAssembly sandbox!
    instance->process_data(42); 
    
    return 0;
}
```

## Advanced Data Marshalling

Cerridwen uses structural annotations to control how data is transferred between native memory and the WebAssembly linear memory block.

* `[[=cerridwen::marshal_by_value{}]]`: Copies the data directly into the WASM linear memory. Suitable for primitives and trivially copyable structs.
* `[[=cerridwen::marshal_by_ref{}]]`: Creates a safe proxy handle inside the WASM module. When WASM accesses the field, it traps back to the host, ensuring the WebAssembly module never directly reads out-of-bounds host memory.

## Building the Framework

Cerridwen utilizes CMake and requires GCC 16 (for `-std=c++26 -freflection`). 

### Windows (MSYS2 / CrossOver)
```cmd
.\build.bat
```

### Linux / macOS
```bash
./build.sh
```

### Docker (Cross-Platform Linux Builds)
To test Linux builds natively on macOS/Windows using Docker:
```bash
./scripts/run_linux_tests_docker.sh
```

## Requirements

* **Compiler**: GCC 16.1.0+ (requires `-freflection` flag for P2996 support).
* **Language Standard**: C++26
* **WebAssembly Runtime**: Wasmtime (or similar C API compliant runtime, fetched automatically via CMake).

## License
Cerridwen is licensed under the GNU Affero General Public License v3.0.
