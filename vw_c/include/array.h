#pragma once

extern "C" {
struct ArrayConstString {
    const char **array;
    int size;
};

ArrayConstString vw_create_array_const_string(const char **input_array,
                                              int size);
void vw_destroy_array_const_string(ArrayConstString array);
}
