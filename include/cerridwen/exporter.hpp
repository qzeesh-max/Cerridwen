#pragma once

#include <cerridwen/annotations.hpp>
#include <iostream>
#include <string_view>
#include <vector>
#include <meta>

namespace cerridwen {

namespace meta_impl {

// Helper to check if a struct/class has an annotation
consteval bool has_annotation(std::meta::info type_info, std::meta::info annot_type) {
    for (auto annot : std::meta::annotations_of(type_info)) {
        if (std::meta::type_of(annot) == annot_type) {
            return true;
        }
    }
    return false;
}

template <typename T>
consteval bool member_has_annotation(std::meta::info mem) {
    for (auto annot : std::meta::annotations_of(mem)) {
        auto t = std::meta::type_of(annot);
        if (t == ^^T) return true;
        std::string_view name = std::meta::display_string_of(t);
        std::string_view target_name = std::meta::identifier_of(^^T); // get just the name
        // string find target name in the type name
        if (name.find(target_name) != std::string_view::npos) return true;
    }
    return false;
}

// Example reflection function that generates WASM bindings
template <typename T>
void generate_wasm_bindings() {
    constexpr auto type_info = ^^T;
    
    constexpr bool is_exported = member_has_annotation<cerridwen::export_wasm>(type_info);
    
    std::cout << "[Cerridwen] Generating WASM bindings for class: " << std::meta::identifier_of(type_info) << "\n";
    if (is_exported) {
        std::cout << "  -> Class is properly annotated with export_wasm\n";
    }
    
    // Check for thread_safe annotation
    if constexpr (member_has_annotation<cerridwen::thread_safe>(type_info)) {
        std::cout << "  -> [Threading] Class marked as thread-safe. Generating WASM locks.\n";
    }

    // Iterate over data members
    constexpr static const auto data_members = std::define_static_array(std::meta::nonstatic_data_members_of(type_info, std::meta::access_context::unchecked()));
    template for (constexpr auto mem : data_members) {
        if constexpr (std::meta::has_identifier(mem)) {
            std::cout << "  -> Discovered Field: " << std::meta::identifier_of(mem) << "\n";
            if constexpr (member_has_annotation<cerridwen::marshal_by_ref>(mem)) {
                std::cout << "     (Marshalling by Reference)\n";
            } else if constexpr (member_has_annotation<cerridwen::marshal_by_value>(mem)) {
                std::cout << "     (Marshalling by Value)\n";
            } else {
                std::cout << "     (Default Marshalling)\n";
            }
        }
    }
    
    // Iterate over methods
    constexpr static const auto all_members = std::define_static_array(std::meta::members_of(type_info, std::meta::access_context::unchecked()));
    template for (constexpr auto mem : all_members) {
        if constexpr (std::meta::is_function(mem) && !std::meta::is_constructor(mem) && !std::meta::is_destructor(mem)) {
            if constexpr (std::meta::has_identifier(mem)) {
                std::string_view name = std::meta::identifier_of(mem);
                if (name != "operator=") {
                    std::cout << "  -> Discovered Method: " << name << "\n";
                }
            }
        }
    }
}

} // namespace meta_impl

} // namespace cerridwen

#define CERRIDWEN_EXPORT_CLASS(T) \
    namespace cerridwen_generated { \
        struct Exporter_##T { \
            Exporter_##T() { \
                ::cerridwen::meta_impl::generate_wasm_bindings<T>(); \
            } \
        }; \
        static Exporter_##T global_exporter_##T; \
    }
