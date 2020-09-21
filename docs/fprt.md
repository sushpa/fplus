# F+ Runtime and Standard Library

## Preface

### Naming

Private members always start with an `_`. Exported members (those that are allowed for use from outside the module) cannot start with an `_`. Base names of any *variables*, *functions* or *types* cannot contain the `_` character at all, and must be in camel case. *Types* and *Enums* must be in capitalized camel case.

The mangled name (or fully-qualified name) is formed by joining the components with an `_`.

E.g. `fp_mem_Pool` is type `Pool` within module `fp.mem`.

## Contents
1. Memory Management
2. Globals
3. Data Structures

## Memory Management

### `fp_mem_alloc()`
### `fp_mem_dealloc()`
### `fp_mem_realloc()`
### `fp_mem_stats()`

