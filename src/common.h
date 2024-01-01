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

// TODO(Al-Andrew): proper s8 struct, and macros
#define ls8$(str) str, sizeof(str) // NOTE(Al-Andrew): only works with bss strings

#endif // CLOX_COMMON_H_INCLUDED