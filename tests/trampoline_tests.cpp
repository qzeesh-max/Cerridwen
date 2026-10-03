#include <gtest/gtest.h>
#include <cerridwen/generator.hpp>
#include <sstream>
#include <string>

struct [[=cerridwen::export_wasm{}]] [[=cerridwen::thread_safe{}]] HeroPlugin {
    virtual ~HeroPlugin() = default;
    
    virtual void attack() = 0;
    virtual void defend() = 0;
    
    [[=cerridwen::marshal_by_ref{}]]
    int health_points;
    
    [[=cerridwen::marshal_by_value{}]]
    int mana;
};

TEST(CerridwenTrampolineTest, GeneratesCorrectProxyCode) {
    std::ostringstream out;
    cerridwen::meta_impl::generate_wasm_trampoline<HeroPlugin>(out, "HeroPlugin");
    std::string generated_code = out.str();
    
    std::cout << "--- GENERATED CODE ---\n" << generated_code << "\n----------------------\n";
    
    // Check for essential generation components
    EXPECT_TRUE(generated_code.find("class HeroPlugin_Trampoline : public HeroPlugin") != std::string::npos);
    EXPECT_TRUE(generated_code.find("cerridwen::WasmInstance* _wasm_instance = nullptr;") != std::string::npos);
    
    // Check for thread-safety handling
    EXPECT_TRUE(generated_code.find("std::mutex _plugin_mutex;") != std::string::npos);
    EXPECT_TRUE(generated_code.find("std::lock_guard<std::mutex> lock(_plugin_mutex);") != std::string::npos);
    
    // Check for method overrides
    EXPECT_TRUE(generated_code.find("void attack() override") != std::string::npos);
    EXPECT_TRUE(generated_code.find("void defend() override") != std::string::npos);
    EXPECT_TRUE(generated_code.find("_wasm_instance->call(\"HeroPlugin_attack\", _wasm_ptr);") != std::string::npos);
    EXPECT_TRUE(generated_code.find("_wasm_instance->call(\"HeroPlugin_defend\", _wasm_ptr);") != std::string::npos);
    
    // Check for marshalling getters
    EXPECT_TRUE(generated_code.find("void sync_health_points()") != std::string::npos);
    EXPECT_TRUE(generated_code.find("void sync_mana()") != std::string::npos);
    EXPECT_TRUE(generated_code.find("[Ref Marshalling]") != std::string::npos);
    EXPECT_TRUE(generated_code.find("[Value Marshalling]") != std::string::npos);
}
struct InterfaceA {
    virtual void a() = 0;
};
struct InterfaceB {
    virtual void b() = 0;
};
struct [[=cerridwen::export_wasm{}]] [[=cerridwen::thread_safe{}]] MultiPlugin : public InterfaceA, public InterfaceB {
    [[=cerridwen::marshal_by_ref{}]]
    int complex_data;
};

TEST(CerridwenTrampolineTest, GeneratesOverridesForMultipleInheritance) {
    std::ostringstream out;
    cerridwen::meta_impl::generate_wasm_trampoline<MultiPlugin>(out, "MultiPlugin");
    std::string generated_code = out.str();

    std::cout << "--- GENERATED CODE ---\n" << generated_code << "\n----------------------\n";

    // Should inherit from MultiPlugin
    EXPECT_TRUE(generated_code.find("class MultiPlugin_Trampoline : public MultiPlugin") != std::string::npos);

    // Should contain overrides for a() and b() inherited from InterfaceA and InterfaceB
    EXPECT_TRUE(generated_code.find("void a() override") != std::string::npos);
    EXPECT_TRUE(generated_code.find("void b() override") != std::string::npos);

    // Threading
    EXPECT_TRUE(generated_code.find("std::mutex _plugin_mutex;") != std::string::npos);
    EXPECT_TRUE(generated_code.find("std::lock_guard<std::mutex> lock(_plugin_mutex);") != std::string::npos);
}
