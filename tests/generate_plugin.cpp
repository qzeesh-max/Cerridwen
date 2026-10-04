#include <iostream>
#include "plugin.hpp"
#include <cerridwen/generator.hpp>

#include <fstream>
#include "host_classes.hpp"

using namespace cerridwen::meta_impl;

// Plugin-exported classes: WASM exports (plugin side) + trampolines (host side).
#define PLUGIN_CLASSES(X) \
    X(MathPlugin) X(Shape) X(Rect) X(Square) X(Walker) X(Swimmer) X(Duck) \
    X(Entity) X(Named) X(Tagged) X(Widget) X(Bird) X(Thrower) X(MemoryProbe) X(StorageClient)

// Host-exported classes: proxies (plugin side) + wasm3 bindings (host side).
#define HOST_CLASSES(X) X(LoggerHost) X(Calculator) X(Animal)

int main() {
    std::ofstream out_exports("tests/plugin_generated.cpp");
    out_exports << "#include \"plugin.hpp\"\n";
    out_exports << "#include \"host_classes.hpp\"\n";
#define X(C) generate_wasm_exports<C>(out_exports, #C);
    PLUGIN_CLASSES(X)
#undef X

    std::ofstream out_host_prox("tests/plugin_host_proxies.hpp");
    out_host_prox << "#pragma once\n";
    out_host_prox << "#include \"host_classes.hpp\"\n";
#define X(C) generate_host_trampoline<C>(out_host_prox, #C);
    HOST_CLASSES(X)
#undef X

    std::ofstream out_tramp("tests/plugin_trampoline.hpp");
    out_tramp << "#pragma once\n";
    out_tramp << "#include \"host_classes.hpp\"\n";
    out_tramp << "#include \"plugin.hpp\"\n";
#define X(C) generate_wasm_trampoline<C>(out_tramp, #C);
    PLUGIN_CLASSES(X)
#undef X
#define X(C) generate_host_exports<C>(out_tramp, #C);
    HOST_CLASSES(X)
#undef X
    return 0;
}
