# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  wasm3
  GIT_REPOSITORY https://github.com/wasm3/wasm3.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(wasm3)

add_executable(cerridwen_tests
    test_main.cpp
    trampoline_tests.cpp
    e2e_tests.cpp
)

target_link_libraries(cerridwen_tests
    PRIVATE cerridwen_core
    gtest_main
    m3
)

# C++26 and reflection
set_target_properties(cerridwen_tests PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cerridwen_tests PRIVATE -freflection)
endif()

add_executable(generate_plugin_tool generate_plugin.cpp)
target_link_libraries(generate_plugin_tool PRIVATE cerridwen_core)
set_target_properties(generate_plugin_tool PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(generate_plugin_tool PRIVATE -freflection)
endif()

# Discover tests
include(GoogleTest)
gtest_discover_tests(cerridwen_tests)
