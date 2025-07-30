#pragma once

/** @file macro.h Common utility macros. */

/** Execute the following block if @p expr is NULL. */
#define CU_IF_NULL(expr) if ((expr) == NULL)
/** Execute the following block if @p expr is not NULL. */
#define CU_IF_NOT_NULL(expr) if ((expr) != NULL)
/** Abort the program with an error message. */
#define CU_DIE(msg) cu_panic_handler("Fatal error: %s", msg)

/** Round @p x up to the nearest multiple of @p align. */
#define CU_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
/** Silence unused variable warnings. */
#define CU_UNUSED(expr) (void)(expr)
/** Array element count. */
#define CU_ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
/** Create a bit mask with bit @p x set. */
#define CU_BIT(x) (1u << (x))

/** Concatenate two tokens after expanding them. */
#define CU_CONCAT_(a, b) a##b
#define CU_CONCAT(a, b) CU_CONCAT_(a, b)

#define CU_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CU_MAX(a, b) ((a) > (b) ? (a) : (b))
 
/** Platform detection macros */
#if defined(_WIN32) || defined(_WIN64) 
#define CU_PLAT_WINDOWS 1
#else
#define CU_PLAT_WINDOWS 0
#endif

#if defined(__APPLE__)
#define CU_PLAT_MACOS 1
#else
#define CU_PLAT_MACOS 0
#endif

#if defined(__linux__)
#define CU_PLAT_LINUX 1
#else
#define CU_PLAT_LINUX 0
#endif

/* WebAssembly platform detection */
#if defined(__EMSCRIPTEN__) || defined(__wasm__) || defined(__wasm32__)
#define CU_PLAT_WASM 1
#else
#define CU_PLAT_WASM 0
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define CU_PLAT_BSD 1
#else
#define CU_PLAT_BSD 0
#endif

#if CU_PLAT_MACOS || CU_PLAT_LINUX || CU_PLAT_BSD
#define CU_PLAT_POSIX 1
#else
#define CU_PLAT_POSIX 0
#endif

#define UNREACHABLE(msg) cu_panic_handler("Unreachable code reached: %s", msg)

#define TODO(msg) cu_panic_handler("TODO: %s", msg)

#define CU_AS(expr, T) (T)(expr)
