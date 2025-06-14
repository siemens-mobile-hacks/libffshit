#ifndef LIBFFSHIT_HELP_H
#define LIBFFSHIT_HELP_H

#include <cstring>

template<typename T>
static char *read_data(char *dst, char *src, size_t size) {
    memcpy(dst, src, sizeof(T) * size);

    return src + sizeof(T) * size;
}

#endif
