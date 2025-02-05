#include "array.h"

ArrayConstString vw_create_array_const_string(const char **input_array,
                                              int size) {
    const char **array =
        static_cast<const char **>(malloc(size * sizeof(const char *)));
    for (int i = 0; i < size; ++i) {
        array[i] = input_array[i];
    }
    return {array, size};
}

void vw_destroy_array_const_string(ArrayConstString array) {
    free(array.array);
}
