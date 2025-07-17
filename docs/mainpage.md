# libcute - C Utility Library

libcute is a comprehensive C utility library designed to provide essential data structures, memory management, and utility functions for C applications.

## Features

### Memory Management
- **Allocator Interface**: Generic allocator interface for consistent memory management
- **Page Allocator**: Efficient page-based memory allocation
- **C Allocator**: Standard C library allocator wrapper
- **GP Allocator**: General-purpose allocator
- **Arena Allocator**: Fast bump-pointer allocator for temporary allocations
- **Slab Allocator**: Efficient fixed-size object allocation

### Collections
- **Vector**: Dynamic arrays with automatic resizing
- **HashMap**: Hash table with customizable hashing and optional lookup
- **Bitmap**: Heap-allocated bit arrays for large dynamic sets
- **Bitset**: Stack-friendly bit arrays with inline storage
- **Ring Buffer**: Circular buffer implementation

### Object System
- **Optional**: Generic optional type for handling nullable values
- **Result**: Generic result type for error handling
- **Slice**: Non-owning view of memory regions

### String Utilities
- **String Buffer**: Dynamic string management
- **String Views**: Efficient string slicing
- **Format Buffer**: String builder and formatting utilities

### Hashing
- **FNV-1A**: Fast non-cryptographic hash function
- **Murmur3**: High-quality hash function
- **SipHash**: Cryptographically secure hash function

### Utility Macros
- **IF_NULL / IF_NOT_NULL**: Conditional execution macros
- **DIE**: Assertion and error handling
- **UNUSED**: Mark unused variables
- **ALIGN_UP**: Memory alignment utilities
- **BIT**: Bit manipulation macros
- **PLAT_X**: Platform detection macros
- **ARRAY_LEN**: Array length calculation
- **UNREACHABLE / TODO**: Code annotation macros

## Usage

To use libcute in your project, include the umbrella header:

```c
#include "cute.h"
```

All public APIs are prefixed with `cu_` followed by the category and function name.

## Building

libcute uses the Meson build system:

```sh
meson setup build
ninja -C build
```

## Testing

Run the test suite:

```sh
ninja -C build test
```

## Documentation

Generate documentation locally:

```sh
ninja -C build docs
```

## Coding Standards

- All public APIs must be prefixed with `cu_`
- Code must be formatted with clang-format
- Headers must be included in the umbrella header `cute.h`
- Filenames use lowercase with underscores for separation

## License

See the project repository for license information.
