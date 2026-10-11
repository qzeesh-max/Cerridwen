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

# Ensure wasm3 uses compiler atomic intrinsics for thread-safe cross-domain atomics
set(M3_ATOMIC_H "${wasm3_SOURCE_DIR}/source/m3_atomic.h")
if(EXISTS "${M3_ATOMIC_H}")
    file(READ "${M3_ATOMIC_H}" _M3_ATOMIC_CONTENT)
    if(NOT _M3_ATOMIC_CONTENT MATCHES "__atomic_fetch_add")
        string(REPLACE
            "static inline\nu64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_value)\n{"
            "static inline\nu64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_value)\n{\n#if defined(__GNUC__) || defined(__clang__)\n    if (i_log2Width == 2) {\n        uint32_t* p = (uint32_t*)io_cell;\n        uint32_t v = (uint32_t)i_value;\n        switch (i_kind) {\n        case c_atomicAdd: return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicSub: return __atomic_fetch_sub(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicAnd: return __atomic_fetch_and(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicOr:  return __atomic_fetch_or(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicXor: return __atomic_fetch_xor(p, v, __ATOMIC_SEQ_CST);\n        default: return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST);\n        }\n    } else if (i_log2Width == 3) {\n        uint64_t* p = (uint64_t*)io_cell;\n        uint64_t v = i_value;\n        switch (i_kind) {\n        case c_atomicAdd: return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicSub: return __atomic_fetch_sub(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicAnd: return __atomic_fetch_and(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicOr:  return __atomic_fetch_or(p, v, __ATOMIC_SEQ_CST);\n        case c_atomicXor: return __atomic_fetch_xor(p, v, __ATOMIC_SEQ_CST);\n        default: return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST);\n        }\n    }\n#endif"
            _M3_ATOMIC_MODIFIED "${_M3_ATOMIC_CONTENT}")
        string(REPLACE
            "static inline\nu64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)\n{"
            "static inline\nu64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)\n{\n#if defined(__GNUC__) || defined(__clang__)\n    if (i_log2Width == 2) {\n        uint32_t* p = (uint32_t*)io_cell;\n        uint32_t exp = (uint32_t)i_expected;\n        __atomic_compare_exchange_n(p, &exp, (uint32_t)i_replacement, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\n        return exp;\n    } else if (i_log2Width == 3) {\n        uint64_t* p = (uint64_t*)io_cell;\n        uint64_t exp = i_expected;\n        __atomic_compare_exchange_n(p, &exp, (uint64_t)i_replacement, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\n        return exp;\n    }\n#endif"
            _M3_ATOMIC_MODIFIED "${_M3_ATOMIC_MODIFIED}")
        file(WRITE "${M3_ATOMIC_H}" "${_M3_ATOMIC_MODIFIED}")
    endif()
endif()

# Full test suite (unit + reflection + end-to-end WASM)
add_executable(cerridwen_tests
    test_main.cpp
    unit_tests.cpp
    trampoline_tests.cpp
    e2e_tests.cpp
)

target_link_libraries(cerridwen_tests
    PRIVATE cerridwen_core
    gtest_main
    m3
)

# Standalone unit test suite (fast unit + reflection tests)
add_executable(cerridwen_unit_tests
    test_main.cpp
    unit_tests.cpp
    trampoline_tests.cpp
)

target_link_libraries(cerridwen_unit_tests
    PRIVATE cerridwen_core
    gtest_main
    m3
)

# C++26 and reflection
set_target_properties(cerridwen_tests PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
set_target_properties(cerridwen_unit_tests PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cerridwen_tests PRIVATE -freflection)
    target_compile_options(cerridwen_unit_tests PRIVATE -freflection)
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
gtest_discover_tests(cerridwen_unit_tests)
