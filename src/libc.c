
/*
 * Lux ACPI Implementation
 * Copyright (C) 2018-2019 by Omar Muhamed
 */

#include <lai/core.h>

void *lai_calloc(size_t count, size_t item_size) {
    size_t size = count * item_size;
    void *p = laihost_malloc(size);
    if(p)
        memset(p, 0, size);
    return p;
}

size_t lai_strlen(const char *s) {
    size_t len = 0;
    for(size_t i = 0; s[i]; i++)
        len++;
    return len;
}

char *lai_strcpy(char *dest, const char *src) {
    char *dest_it = (char *)dest;
    const char *src_it = (const char *)src;
    while(*src_it)
        *(dest_it++) = *(src_it++);
    *dest_it = 0;
    return dest;
}

int lai_strcmp(const char *a, const char *b) {
    size_t i = 0;
    while(1) {
        unsigned char ac = a[i];
        unsigned char bc = b[i];
        if(!ac && !bc)
            return 0;
        // If only one char is null, one of the following cases applies.
        if(ac < bc)
            return -1;
        if(ac > bc)
            return 1;
        i++;
    }
}

