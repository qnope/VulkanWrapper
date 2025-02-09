#pragma once

extern "C" {
struct vw_ArrayConstString {
    const char **array;
    int size;
};

vw_ArrayConstString vw_create_array_const_string(const char *const *input_array,
                                                 int size);
void vw_destroy_array_const_string(vw_ArrayConstString array);
}
