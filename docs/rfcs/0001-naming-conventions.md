# RFC: Naming Conventions

This document proposes the naming guidelines for libcute. All public symbols must share a common prefix and follow consistent casing rules.

## Prefix

All exported types, functions and global variables begin with `cu_`. The prefix is followed by the subsystem or category name and then the actual identifier.

Examples:

- `cu_Allocator_PageAllocator`
- `cu_String_create`

## Casing

- Types, functions and enum names use `CamelCase`.
- Enum constants are `UPPER_CASE`.
- Variables and struct members use `lower_case`.
- File names are lowercase, using underscores to separate words.

## Enforcement

The `.clang-tidy` configuration contains identifier naming checks to enforce these rules automatically.
