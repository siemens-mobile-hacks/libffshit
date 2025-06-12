#ifndef HELP_H
#define HELP_H

#include <cstring>

template<typename T>
static char *read_data(char *dst, char *src, size_t size) {
    memcpy(dst, src, sizeof(T) * size);

    return src + sizeof(T) * size;
}

#endif
