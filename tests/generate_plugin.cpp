#include <iostream>
#include "plugin.hpp"
#include <cerridwen/generator.hpp>

#include <fstream>
#include "host_classes.hpp"

int main() {
    std::ofstream out_exports("tests/plugin_generated.cpp");
    out_exports << "#include \"plugin.hpp\"\n";
    out_exports << "#include \"host_classes.hpp\"\n";
    cerridwen::meta_impl::generate_wasm_exports<MathPlugin>(out_exports, "MathPlugin");
    
    std::ofstream out_host_prox("tests/plugin_host_proxies.hpp");
    out_host_prox << "#pragma once\n";
    out_host_prox << "#include \"host_classes.hpp\"\n";
    cerridwen::meta_impl::generate_host_trampoline<LoggerHost>(out_host_prox, "LoggerHost");
    
    std::ofstream out_tramp("tests/plugin_trampoline.hpp");
    out_tramp << "#pragma once\n";
    out_tramp << "#include \"host_classes.hpp\"\n";
    cerridwen::meta_impl::generate_wasm_trampoline<MathPlugin>(out_tramp, "MathPlugin");
    cerridwen::meta_impl::generate_host_exports<LoggerHost>(out_tramp, "LoggerHost");
    return 0;
}
