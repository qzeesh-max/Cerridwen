#pragma once

#include <gtest/gtest.h>

#include <cerridwen/wasm3_instance.hpp>

#include "host_classes.hpp"
#include "plugin.hpp"
#include "plugin_trampoline.hpp"  // generated: <Class>_Trampoline + bind_<HostClass>

#include <filesystem>
#include <memory>
#include <random>
#include <string>

namespace e2e {

inline std::string wasm_path() {
    for (const char* p : {"plugin.wasm", "tests/plugin.wasm", "build/tests/plugin.wasm"}) {
        if (FILE* f = fopen(p, "rb")) { fclose(f); return p; }
    }
    return "build/tests/plugin.wasm";
}

// Loads the test plugin and binds every host class the plugin may call into.
inline std::unique_ptr<cerridwen::Wasm3Instance> load(cerridwen::InstanceOptions opts = {}) {
    auto inst = std::make_unique<cerridwen::Wasm3Instance>(wasm_path(), opts);
    bind_LoggerHost(inst->get_module());
    bind_Calculator(inst->get_module());
    bind_Animal(inst->get_module());
    return inst;
}

// Creates the plugin-side object and wraps it in its host-side trampoline.
template <class Trampoline>
std::unique_ptr<Trampoline> spawn(cerridwen::Wasm3Instance& inst, const char* create_fn) {
    auto t = std::make_unique<Trampoline>();
    t->_wasm_instance = &inst;
    t->_wasm_ptr = inst.call_create(create_fn);
    return t;
}

inline uint64_t addr(void* p) { return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)); }

// Base fixture: one fresh plugin instance per test.
class PluginTest : public ::testing::Test {
protected:
    std::unique_ptr<cerridwen::Wasm3Instance> inst;
    void SetUp() override { inst = load(); }
    // Trampolines must be destroyed before the instance they point at, so tests
    // declare them locally (they die before TearDown resets `inst`).
};

// Fixture with a throw-away storage directory.
class StorageTest : public ::testing::Test {
protected:
    std::filesystem::path base;   // parent that contains the jail
    std::filesystem::path jail;   // the host-chosen storage root
    void SetUp() override {
        std::random_device rd;
        base = std::filesystem::temp_directory_path() / ("cerridwen_" + std::to_string(rd()) + "_" + std::to_string(rd()));
        jail = base / "jail";
        std::filesystem::create_directories(base);
    }
    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(base, ec);
    }
};

} // namespace e2e
