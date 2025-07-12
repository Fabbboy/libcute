## libcute

libcute is a c-utility library.

memory-features:

- [x] Allocator interface
- [x] Page Allocator
- [x] C Allocator
- [x] GP Allocator
- [ ] Slab Allocator
- [ ] Arena Allocator
- [ ] Fixed Allocator

macro-features:

- [x] IF_NULL
- [x] IF_NOT_NULL
- [x] DIE
- [x] UNUSED
- [x] ALIGN_UP
- [x] BIT
- [x] PLAT_X (platform macros)
- [x] ARRAY_LEN
- [x] UNREACHABLE(msg) (with line and file)
- [x] TODO(msg) (with line and file)

object-features:

- [x] generic optional
- [x] generic result
- [x] slice (non-owning view of memory)
- [ ] configurable passable (non global) logger
- [ ] string builder
- [ ] generic error interface

string-features:

- [x] string buffer
- [x] string views
- [ ] string utility methods (maybe powered by simd. crossplat fallbacks very important)

collection-features:

- [x] vector
- [x] hashmap (customizable hashing and optional lookup)
- [x] bitset (local)
- [x] bitmap (heaped)
      Bitsets keep their storage inline and provide fast, stack-friendly access.
      Bitmaps allocate their storage on the heap and are used for larger dynamic sets.
- [ ] linked and doubly linked
- [x] ring buffer

method-features:

- [x] hashing methods FNV-1A, Murmur3, SipHash (tied to hashmap, stil separate)

## Todo's

- [x] Rename optional generated methods to include `Optional` in the name

## Allocators

All methods or datastructures that need an allocator should accept the generic allocator interface in a zig style

## Coding standards

Code must be written and formatted in a `clang-format`-friendly way.

All methods and structs that do not represent standard library types (e.g. `Int_Optional`) must be prefixed with `cu_`. This prefix is followed by the category (e.g. `Allocator`) and finally the actual type or function name.

Example:

- `PageAllocator` → `cu_Allocator_PageAllocator`

The same naming convention applies to functions.

Filenames must be lowercase. If the filename contains multiple words, use underscores (`_`) to separate them.

All headers must be included in the `cute.h` umbrella header.

Utility types and methods that do not belong to any specific subsystem can be placed in the root directory.

Files that can be implemented in C should **not** use header-based implementations (i.e., avoid `static inline` unless necessary or otherways useless).

If additional types are required for implementation or if you come up with something useful, add it to the list above.

For structs which are exposed to the user but are not intended to be used should use `struct T` instead of `typedef struct`. But naming stays the same

## Commit Style

This project does **not** enforce conventional commits, but commits should follow a minimal style that helps generate meaningful changelogs.

### 🧱 Format Guidelines

- Start with the general area or subsystem being modified (e.g., `allocator`, `macro`, `vector`)
- Be short but descriptive
- Use lowercase only for prefixes
- Avoid "Update", "Changes", or generic nonsense

#### ✅ Good examples

```
allocator: implement aligned page allocator
macro: add CU_ARRAY_LEN
slice: refactor to support zero-length spans
hashmap: stub file with header structure
simd: add platform dispatch detection
```

#### ❌ Bad examples

```
update stuff
misc fixes
more work
final version
```

This style is compatible with the changelog generator (`git-cliff.toml`) and ensures readable release notes.

## Documentation

Documentation is generated with Doxygen. When changes are pushed to the `main`
branch, GitHub Actions and GitLab CI build the docs and publish them through
GitHub Pages and GitLab Pages. Locally you can run:

```sh
meson setup build
ninja -C build docs
```
