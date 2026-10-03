#pragma once

namespace cerridwen {

// Primary annotation to export a C++ struct/class to WebAssembly
struct export_wasm {};

// Data marshalling annotations
struct marshal_by_value {};
struct marshal_by_ref {};

// Threading model annotations
struct thread_safe {};

// Method specific annotations (e.g., exposing specific methods)
struct export_method {};

// Primary annotation to export a C++ struct/class from the Host to WebAssembly
struct export_host {};

// Annotation for storage classes
struct storage_class {};

} // namespace cerridwen

#if defined(__cpp_reflection)
    #define CERRIDWEN_EXPORT_WASM [[=cerridwen::export_wasm{}]]
    #define CERRIDWEN_EXPORT_HOST [[=cerridwen::export_host{}]]
    #define CERRIDWEN_THREAD_SAFE [[=cerridwen::thread_safe{}]]
    #define CERRIDWEN_MARSHAL_BY_VALUE [[=cerridwen::marshal_by_value{}]]
    #define CERRIDWEN_MARSHAL_BY_REF [[=cerridwen::marshal_by_ref{}]]
    #define CERRIDWEN_EXPORT_METHOD [[=cerridwen::export_method{}]]
    #define CERRIDWEN_STORAGE_CLASS [[=cerridwen::storage_class{}]]
#else
    #define CERRIDWEN_EXPORT_WASM
    #define CERRIDWEN_EXPORT_HOST
    #define CERRIDWEN_THREAD_SAFE
    #define CERRIDWEN_MARSHAL_BY_VALUE
    #define CERRIDWEN_MARSHAL_BY_REF
    #define CERRIDWEN_EXPORT_METHOD
    #define CERRIDWEN_STORAGE_CLASS
#endif
