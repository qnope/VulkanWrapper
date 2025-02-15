#pragma once

extern "C" {
struct VwArrayConstString {
    const char **array;
    int size;
};

VwArrayConstString vw_create_array_const_string(const char *const *input_array,
                                                int size);
void vw_destroy_array_const_string(const VwArrayConstString *array);
}
