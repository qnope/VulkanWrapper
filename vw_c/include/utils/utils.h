#pragma once

extern "C" {
struct String {
    const char *string;
};

struct StringArray {
    String *array;
    int number;
};
}
