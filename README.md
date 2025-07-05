## libcute 
libcute is a c-utility library.

memory-features:
 - [X] Allocator interface
 - [X] Page Allocator 
 - [X] C Allocator
 - [ ] GP Allocator 
 - [ ] Slab Allocator
 - [ ] Arena Allocator 

macro-features:
 - [X] IF_NULL
 - [X] IF_NOT_NULL
 - [X] DIE 
 - [X] UNUSED
 - [X] ALIGN_UP
 - [ ] BIT 
 - [ ] PLAT_X (platform macros)
 - [ ] ARRAY_LEN

object-features:
 - [X] generic optional
 - [X] generic result
 - [X] slice
 - [ ] configurable passable (non global) logger 
 - [ ] string builder
 - [ ] generic error interface

 string-features:
 - [ ] string buffer
 - [ ] string views
 - [ ] string utility methods (maybe powered by simd)
 - [ ] utf8 utility methods

collection-features:
 - [ ] vector
 - [ ] hashmap
 - [ ] bitmap/bitset
 - [ ] linked and doubly linked 
 - [ ] ring buffer 

 method-features:
 - [ ] hashing methods FNV-1A, Murmur3, SipHash (tied to hashmap, stil separate)
 
## Coding standards
Code must be written and formatted in a `clang-format`-friendly way.

All methods and structs that do not represent standard library types (e.g. `Int_Optional`) must be prefixed with `cu_`. This prefix is followed by the category (e.g. `Allocator`) and finally the actual type or function name.

Example:
- `PageAllocator` → `cu_Allocator_PageAllocator`

The same naming convention applies to functions.

Filenames must be lowercase. If the filename contains multiple words, use underscores (`_`) to separate them.

All headers must be included in the `cute.h` umbrella header.

Utility types and methods that do not belong to any specific subsystem can be placed in the root directory.

Files that can be implemented in C should **not** use header-based implementations (i.e., avoid `static inline` unless necessary).

If additional types are required for implementation or if you come up with something useful, add it to the list above.

