#ifndef CLOX_COMMON_H_INCLUDED
#define CLOX_COMMON_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(X)

#define CLOX_DEV_ASSERT(exp) { assert((exp)); }
#define CLOX_UNREACHABLE() { assert(false); }


typedef struct {
    const char* string;
    int len;
} s8;

#define s8$(str) (s8){.string = str, .len = sizeof(str) - 1}
#define s8d$(str) (s8){.string = str, .len = strlen(str)}
// TODO(Al-Andrew): proper s8 struct, and macros
#define ls8$(str) str, (sizeof(str) - 1) // NOTE(Al-Andrew): only works with bss strings

int s8_compare(s8 s1, s8 s2);

#endif // CLOX_COMMON_H_INCLUDED