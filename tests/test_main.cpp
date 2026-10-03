#include <gtest/gtest.h>
#include <cerridwen/exporter.hpp>
#include <cerridwen/plugin_manager.hpp>
#include <string>

// Test Subject: A simple class exposed to WASM
struct [[=cerridwen::export_wasm{}]] BasePlugin {
    virtual ~BasePlugin() = default;
    
    virtual void on_init() = 0;
    
    [[=cerridwen::marshal_by_value{}]]
    int simple_data;
};

CERRIDWEN_EXPORT_CLASS(BasePlugin);

// Test Subject: Multiple Inheritance
struct InterfaceA { virtual void a() = 0; };
struct InterfaceB { virtual void b() = 0; };

struct [[=cerridwen::export_wasm{}]] [[=cerridwen::thread_safe{}]] MultiPlugin : public InterfaceA, public InterfaceB {
    virtual ~MultiPlugin() = default;
    
    void a() override {}
    void b() override {}
    
    [[=cerridwen::marshal_by_ref{}]]
    std::string complex_data;
};

CERRIDWEN_EXPORT_CLASS(MultiPlugin);

TEST(CerridwenReflectionTest, MacroGeneratesOutput) {
    // This test ensures the reflection code compiles and runs.
    // In a real framework, we'd verify the generated AST or trampoline objects.
    cerridwen::PluginManager manager;
    auto plugin = manager.load_plugin("dummy.wasm");
    EXPECT_NE(plugin, nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
