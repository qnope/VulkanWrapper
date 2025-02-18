#pragma once

extern "C" {
struct VwString {
    const char *string;
};

struct VwStringArray {
    const VwString *array;
    int number;
};
}
