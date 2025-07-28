# RFC: Naming Conventions

This document proposes the naming guidelines for libcute. All public symbols must share a common prefix and follow consistent casing rules.

## Prefix

All exported identifiers begin with `cu_`. The prefix is followed by a
`Category` and optionally a `Type` name, both written in `PascalCase`. Function
methods come last and use `lower_case`.

The pattern is `cu_<Category>_<Type?>_<method>` where `<Type>` is omitted if not applicable.

Examples:

- `cu_File_open`
- `cu_File_OpenOptions_write`

## Casing

- Types, functions and enum names use `CamelCase`.
- Enum constants are `UPPER_CASE`.
- Variables and struct members use `lower_case`.
- File names are lowercase, using underscores to separate words.

## Enforcement

The `.clang-tidy` configuration contains identifier naming checks to enforce these rules automatically.
