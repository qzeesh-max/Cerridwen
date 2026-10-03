#include <meta>
#include <iostream>
#include <vector>
#include <string_view>

struct Foo {
    void multiply(int factor, double y);
};

int main() {
    constexpr auto type_info = ^^Foo;
    constexpr auto members = std::meta::nonstatic_data_members_of(type_info, std::meta::access_context::unchecked());
    // find method
    constexpr static const auto all_members = std::define_static_array(std::meta::members_of(type_info, std::meta::access_context::unchecked()));
    template for (constexpr auto mem : std::define_static_array(all_members)) {
        if constexpr (std::meta::is_function(mem) && !std::meta::is_constructor(mem) && !std::meta::is_destructor(mem) && std::meta::has_identifier(mem)) {
            std::cout << "Function: " << std::meta::identifier_of(mem) << "\n";
            // print parameters
            // constexpr auto params = std::meta::parameters_of(mem);
            // wait, P2996 says it might be std::meta::parameters_of(std::meta::type_of(mem)) or something. Let's just try type_of(mem).
            // Actually, wait, functions themselves don't have parameters, function types do? No, functions do.
            constexpr auto type_t = std::meta::type_of(mem);
            // wait, we can just do type_of(mem) and look at the return type and params
            template for (constexpr auto p : std::define_static_array(std::meta::parameters_of(type_t))) {
                std::cout << " Param: " << std::meta::display_string_of(p) << "\n";
            }
            std::cout << " Type string: " << std::meta::display_string_of(type_t) << "\n";
            std::cout << " Return type: " << std::meta::display_string_of(std::meta::return_type_of(type_t)) << "\n";
        }
    }
}
