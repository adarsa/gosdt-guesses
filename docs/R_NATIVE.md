# R-native GOSDT (no Python)

This document is for **using or packaging GOSDT from R only**. It assumes **no Python runtime**, **no reticulate**, and **no PyPI wheels**—only the C++ library in this repository plus R bindings and R code you add in a separate R package (or local `src/` build).

---

## 1. What you are reproducing

| Layer | In this repo today | R-only replacement |
|--------|-------------------|---------------------|
| Solver | C++ under `src/libgosdt/` | **Reuse as-is** (same sources) |
| Foreign bindings | `src/libgosdt.cpp` (pybind11) | **New**: Rcpp or cpp11 glue in the R package `src/` |
| Public API | `GOSDTClassifier`, binarizers, `Tree` JSON parsing | **R functions + S3 classes** reimplementing that logic |
| Build | CMake + scikit-build (Python) | **R**: `Makevars` / `configure` (and optional CMake invoked from `configure`) |

The Python package only **prepares matrices, configuration, and labels** and calls `gosdt_fit(dataset)`; all of that can live in pure R plus a thin native call.

---

## 2. Task list (implementation)

Use this as a project checklist for the **new R package** (suggested top-level name: e.g. `gosdt`, subject to CRAN naming availability).

### Core native interface

1. **Vendor or submodule** the C++ tree: copy `src/libgosdt/` (headers + sources) into the R package `src/gosdt_core/` (or similar), excluding `src/libgosdt.cpp` (pybind11).
2. **Implement R bindings** (Rcpp or cpp11) that expose the minimum surface needed:
   - Build `Configuration` from R (lists or explicit arguments).
   - Build `Dataset` from R matrices (logical/float) and the feature map (`list` of integer sets).
   - Call `gosdt::fit` and return model string, bounds, timings, iteration counts, status enum.
3. **Matrix layout**: match the Python convention (boolean input matrix collated with binarized labels as in `_classifier.py`); document row/column semantics in `man/` pages.
4. **nlohmann/json**: the upstream `CMakeLists.txt` uses `FetchContent` (network at configure). For R builds you should **vendor** a single-header or small `include/` copy so **offline** and **CRAN-style** installs do not download during `R CMD INSTALL`.

### Pure R layer

5. **Configuration defaults and validation** mirroring `GOSDTClassifier` (regularization floor, `depth_budget` off-by-one vs C++, `time_limit`, flags, etc.).
6. **Label handling**: multiclass → boolean label matrix; binary edge case (two columns); optional reference labels `y_ref` → `reference_LB`.
7. **Cost matrix**: default and balanced variants as in Python; accept optional user matrix.
8. **Feature map**: default trivial map `[[1]], [[2]], ...` in R terms (sets of 1-based indices consistent with R).
9. **Output parsing**: parse the JSON model string into an in-memory structure and implement `predict` / `predict_proba` equivalent (port `_tree.py` logic).
10. **Binarization** (if you expose it): port or replace `NumericBinarizer` / `ThresholdGuessBinarizer` in R, or document that users must supply logical matrices from other tools.

### Quality and distribution

11. **Unit tests** (`testthat`): parity with `tests/test_*.py` where feasible; `skip_on_cran()` for long runs.
12. **`SystemRequirements`**: C++20, GNU GMP, oneTBB; document versions tested.
13. **Examples**: small dataset only; avoid long optimization in `\examples{}` on CRAN.
14. **Licensing**: BSD-3-Clause for your glue code; record third-party licenses if you **bundle** GMP/TBB sources or prebuilt Windows libs.

---

## 3. Considerations (must-read)

### Dependencies

- **C++20** compiler (g++ or clang new enough for your target R version).
- **GMP** and **Intel oneTBB** must be available at **link** time unless you fully static-link them (common on Windows; on Linux, distro `-dev` packages are typical).

### Windows vs Linux servers

- **Linux server**: install development packages (package names vary):
  - Debian/Ubuntu-style: `libgmp-dev`, `libtbb-dev` (or `onetbb` / `libtbb-dev` depending on distro).
  - RHEL/CentOS/Fedora: `gmp-devel`, `tbb-devel` / `onetbb-devel`.
  - Ensure **pkg-config** can find GMP if your `configure` uses it.
- **Windows (Rtools)**: R builds with **MinGW-w64**, not MSVC. Do **not** assume **vcpkg MSVC** artifacts link cleanly with R’s g++. Prefer **Rtools-provided** or **prebuilt MinGW static libraries** with matching ABI (see section 5).

### Threading and HPC

- `worker_limit > 1` is known to be risky in the current Python release (deadlock warning). Treat the same as **experimental** in R; default to single worker unless you validate on your cluster.
- On shared servers, document CPU usage and respect scheduler/memory limits.

### Reproducibility

- Pin a **git tag or commit** when cloning this repository for production.
- Record compiler, GMP, and TBB versions in your R package `README` or `inst/NEWS` for support.

### CRAN (if you submit later)

- No network downloads during `R CMD check` / install (vendored JSON, no live `FetchContent`).
- Keep check time and example runtime within policy; use `\donttest{}` / skips appropriately.

---

## 4. Setup: Linux server (typical)

### 4.1 Install system libraries and R

Example for **Debian/Ubuntu**:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake pkg-config \
  libgmp-dev \
  libtbb-dev \
  r-base r-base-dev
```

Adjust package names for RHEL/Fedora/OpenSUSE as needed.

### 4.2 R package build variables (illustrative)

Your R package should discover flags via `configure` or document manual `~/.R/Makevars`:

```make
# Illustrative only — paths differ by distro
PKG_CPPFLAGS = $(shell pkg-config --cflags gmp tbb)
PKG_LIBS     = $(shell pkg-config --libs gmp tbb)
```

If `pkg-config` does not expose TBB on your distro, set `-I` / `-L` from vendor documentation (e.g. CMake config paths for oneTBB).

### 4.3 Install the future R package

Once the R package exists:

```r
# From a local directory after cloning your R package repo
install.packages("path/to/gosdt", repos = NULL, type = "source")
```

No Python should be involved in this path.

---

## 5. Setup: Windows (Rtools)

1. Install **R** and **Rtools** matching your R version (UCRT toolchain for current R).
2. Install **GMP** and **TBB** in a form **linkable by MinGW g++**:
   - Prefer **static** `.a` libraries bundled with the R package under e.g. `src/win-libs/`, **or**
   - MSYS2 packages inside Rtools that match the compiler ABI, with `Makevars.win` pointing to them.
3. Do **not** rely on MSVC-only vcpkg builds for the default CRAN-style R package compile.

Exact `Makevars.win` lines depend on where you place headers and libs; your R package maintainer should ship a tested template.

---

## 6. Setup: macOS

```bash
# Homebrew (Apple Silicon or Intel)
brew install gmp tbb pkg-config
```

Point `PKG_CPPFLAGS` / `PKG_LIBS` via `configure` or `~/.R/Makevars` using `pkg-config` output. Ensure Xcode Command Line Tools are installed.

---

## 7. Verification (no Python)

After the R package is installed:

1. Run its **unit tests**: `testthat::test_package("gosdt")` (or your package name).
2. Fit a **tiny** logical matrix problem and check that `predict` matches a known baseline you store in `tests/testthat/`.
3. Confirm **no** `python`, `reticulate`, or `.py` files are required on `library()` load.

---

## 8. Scaffold in this repository

An experimental package lives at **`r/gosdt/`** in this monorepo. It:

- Runs **`configure`** on Unix to locate GMP/TBB via **pkg-config**, write **`src/Makevars`**, and create **`src/gosdt_iface_*.cpp` symlinks** into `src/libgosdt/src/**/*.cpp` (R’s build only compiles `*.cpp` that live under `src/`).
- Vendors **`inst/include/nlohmann/json.hpp`** so the C++ headers resolve without CMake `FetchContent`.
- Links **`-ltbbmalloc`** in addition to **`-ltbb`** (matches `CMakeLists.txt`; avoids missing `scalable_free` at `dyn.load` time on macOS/Linux).

Install from the repository root:

```bash
R CMD INSTALL r/gosdt
```

Replace the placeholder **Maintainer** in `r/gosdt/DESCRIPTION` before any CRAN submission.

## 9. Relation to this repository

This GitHub project remains the **canonical C++ implementation** and Python packaging. The R layer in **`r/gosdt/`** reuses `src/libgosdt/` in-tree instead of the pybind11 shim (`src/libgosdt.cpp`).

For algorithm references and hyperparameter meaning, see the root `README.md` and the papers cited there.
