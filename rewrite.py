import sys

with open("include/cerridwen/generator.hpp", "r") as f:
    code = f.read()

# Replace the serialization logic block.
# We will look for: if (p_type.find("std::vector") != std::string::npos && p_type.find("&") != std::string::npos) {
# and replace until: } else if (p_type.find("std::shared_ptr") != std::string::npos) {

start_str = 'if (p_type.find("std::vector") != std::string::npos && p_type.find("&") != std::string::npos) {'
end_str = '} else if (p_type.find("std::shared_ptr") != std::string::npos) {'

start_idx = code.find(start_str)
end_idx = code.find(end_str, start_idx)

if start_idx == -1 or end_idx == -1:
    print("Could not find block 1")
    sys.exit(1)

new_block1 = """if (p_type.find("std::") != std::string::npos && p_type.find("&") != std::string::npos && p_type.find("shared_ptr") == std::string::npos) {
                                serialize_pre_call += 
                                    "        cerridwen::Buffer _buf_" + arg_name + ";\\n"
                                    "        cerridwen::serialize(_buf_" + arg_name + ", " + arg_name + ");\\n"
                                    "        uint32_t " + arg_name + "_struct = _wasm_instance->allocate(8);\\n"
                                    "        uint32_t " + arg_name + "_data = _wasm_instance->allocate(_buf_" + arg_name + ".data.size());\\n"
                                    "        _wasm_instance->write_memory(" + arg_name + "_data, _buf_" + arg_name + ".data.data(), _buf_" + arg_name + ".data.size());\\n"
                                    "        uint32_t " + arg_name + "_len = _buf_" + arg_name + ".data.size();\\n"
                                    "        _wasm_instance->write_memory(" + arg_name + "_struct, &" + arg_name + "_data, 4);\\n"
                                    "        _wasm_instance->write_memory(" + arg_name + "_struct + 4, &" + arg_name + "_len, 4);\\n"
                                    "        _args.push_back(std::to_string(" + arg_name + "_struct));\\n";

                                serialize_post_call +=
                                    "        uint32_t new_" + arg_name + "_data = 0;\\n"
                                    "        uint32_t new_" + arg_name + "_len = 0;\\n"
                                    "        _wasm_instance->read_memory(" + arg_name + "_struct, &new_" + arg_name + "_data, 4);\\n"
                                    "        _wasm_instance->read_memory(" + arg_name + "_struct + 4, &new_" + arg_name + "_len, 4);\\n"
                                    "        cerridwen::Buffer _out_buf_" + arg_name + ";\\n"
                                    "        _out_buf_" + arg_name + ".data.resize(new_" + arg_name + "_len);\\n"
                                    "        _wasm_instance->read_memory(new_" + arg_name + "_data, _out_buf_" + arg_name + ".data.data(), new_" + arg_name + "_len);\\n"
                                    "        cerridwen::deserialize(_out_buf_" + arg_name + ", " + arg_name + ");\\n"
                                    "        _wasm_instance->deallocate(" + arg_name + "_struct);\\n"
                                    "        _wasm_instance->deallocate(" + arg_name + "_data);\\n"
                                    "        if (new_" + arg_name + "_data != " + arg_name + "_data) _wasm_instance->deallocate(new_" + arg_name + "_data);\\n";
                            """

code = code[:start_idx] + new_block1 + code[end_idx:]

# Now replace the deserialization logic block.
# We will look for: if (p_type.find("std::vector") != std::string::npos && p_type.find("&") != std::string::npos) {
# and replace until: } else if (p_type.find("std::shared_ptr") != std::string::npos) {

start_str2 = 'if (p_type.find("std::vector") != std::string::npos && p_type.find("&") != std::string::npos) {'
end_str2 = '} else if (p_type.find("std::shared_ptr") != std::string::npos) {'

start_idx2 = code.find(start_str2, start_idx + 100)
end_idx2 = code.find(end_str2, start_idx2)

if start_idx2 == -1 or end_idx2 == -1:
    print("Could not find block 2")
    sys.exit(1)

new_block2 = """if (p_type.find("std::") != std::string::npos && p_type.find("&") != std::string::npos && p_type.find("shared_ptr") == std::string::npos) {
                            param_list_decl += ", uint32_t " + arg_name;
                            param_list_call += "local_" + arg_name;
                            
                            deserialize_pre_call +=
                                "    using BaseT_" + arg_name + " = std::remove_reference_t<" + p_type + ">;\\n"
                                "    uint32_t* " + arg_name + "_struct = reinterpret_cast<uint32_t*>(" + arg_name + ");\\n"
                                "    uint32_t " + arg_name + "_data = " + arg_name + "_struct[0];\\n"
                                "    uint32_t " + arg_name + "_len = " + arg_name + "_struct[1];\\n"
                                "    cerridwen::Buffer _buf_" + arg_name + ";\\n"
                                "    _buf_" + arg_name + ".data.assign((uint8_t*)" + arg_name + "_data, (uint8_t*)" + arg_name + "_data + " + arg_name + "_len);\\n"
                                "    BaseT_" + arg_name + " local_" + arg_name + ";\\n"
                                "    cerridwen::deserialize(_buf_" + arg_name + ", local_" + arg_name + ");\\n";
                            
                            serialize_post_call +=
                                "    cerridwen::Buffer _out_buf_" + arg_name + ";\\n"
                                "    cerridwen::serialize(_out_buf_" + arg_name + ", local_" + arg_name + ");\\n"
                                "    uint8_t* new_" + arg_name + "_data = (uint8_t*) cerridwen::plugin::allocate(_out_buf_" + arg_name + ".data.size());\\n"
                                "    std::memcpy(new_" + arg_name + "_data, _out_buf_" + arg_name + ".data.data(), _out_buf_" + arg_name + ".data.size());\\n"
                                "    " + arg_name + "_struct[0] = reinterpret_cast<uint32_t>(new_" + arg_name + "_data);\\n"
                                "    " + arg_name + "_struct[1] = _out_buf_" + arg_name + ".data.size();\\n";
                        """

code = code[:start_idx2] + new_block2 + code[end_idx2:]

# Now we must insert `#include "cerridwen/serializer.hpp"` near the top of the generated code for both host and WASM.
# Wait, for the host, we can add it to the include block.
incl_str = 'out << "#include \\"<vector>\\"\\n";'
code = code.replace(incl_str, incl_str + '\\n    out << "#include \\"cerridwen/serializer.hpp\\"\\n";')

incl_wasm_str = 'out << "#include <emscripten.h>\\n\\n";'
code = code.replace(incl_wasm_str, incl_wasm_str + '    out << "#include \\"cerridwen/serializer.hpp\\"\\n\\n";')

with open("include/cerridwen/generator.hpp", "w") as f:
    f.write(code)

print("Done")
