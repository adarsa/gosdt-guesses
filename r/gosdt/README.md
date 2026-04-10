# gosdt (R package, no Python)

This directory is an **R-native** scaffold that links the C++ core at `../../src/libgosdt`. It does **not** use Python or reticulate.

## Requirements

- Full **gosdt-guesses** repository checkout (`r/gosdt` must be two levels below the repo root).
- **R** ≥ 4.2, **Rcpp**, **pkg-config**.
- **C++20** compiler, **GNU GMP**, **Intel oneTBB** (development packages).

### Linux (example: Debian/Ubuntu)

```bash
sudo apt-get install -y build-essential pkg-config libgmp-dev libtbb-dev r-base-dev
R -e 'install.packages("Rcpp")'
```

### macOS (Homebrew)

```bash
brew install gmp tbb pkg-config
```

### Windows

Automatic configuration is **not** provided. Use **Rtools** (MinGW-w64) and edit `src/Makevars.win` with correct `-I` / `-L` flags and library names for GMP and TBB built for that toolchain. See `../docs/R_NATIVE.md`.

## Install from this repository

```bash
cd /path/to/gosdt-guesses
R CMD INSTALL r/gosdt
```

`configure` runs during installation, writes `src/Makevars`, and creates **`src/gosdt_iface_*.cpp` symbolic links** to every `../../src/libgosdt/src/**/*.cpp` file. R’s package build only compiles `*.cpp` that live under `src/`, so this avoids a custom `OBJECTS` list (which R may ignore). **`cleanup`** removes those symlinks and `Makevars`.

**Linking:** `PKG_LIBS` includes **both `-ltbb` and `-ltbbmalloc`**, matching the upstream CMake project (needed for `scalable_free` at load time).

## Vendored headers

- `inst/include/nlohmann/json.hpp` — [nlohmann/json](https://github.com/nlohmann/json) (matches upstream C++ includes).

## After changing `// [[Rcpp::export]]` functions

From the package root:

```r
Rcpp::compileAttributes(".")
```

Then reinstall.

## Status

- **Done:** Build/link against full `libgosdt`, placeholder export `gosdt_lib_version()`.
- **Todo:** R-level classifier API, `gosdt::fit` wrappers, tests against small problems. See `../docs/R_NATIVE.md`.
