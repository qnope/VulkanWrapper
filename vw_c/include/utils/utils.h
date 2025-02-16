#pragma once

extern "C" {
struct VwString {
    const char *string;
};

struct VwStringArray {
    VwString *array;
    int number;
};
}
