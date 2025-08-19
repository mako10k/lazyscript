# Agent Instructions

To build this project, ensure the following packages are installed:

- `build-essential` (provides `gcc`, `g++`, `make`)
- `autoconf`
- `automake`
- `libtool`
- `pkg-config`
- `flex` (>= 2.6)
- `bison` (>= 3.x)
- `libgc-dev` (Boehm-Demers-Weiser garbage collector)
- `doxygen` and `graphviz` (optional, for documentation)

Typical build steps:

```sh
autoreconf -i
./configure
make -j$(nproc)
```
