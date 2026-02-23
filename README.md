# cc-compression-tool-cpp

Huffman encoder/decoder built in C++ (C++23), based on [Coding Challenges](https://codingchallenges.fyi/) by John Crickett.

---

## Dependencies

| Dependency | Version / notes |
|------------|------------------|
| **CMake** | 3.15 or later |
| **Build system** | Ninja (used by presets) |
| **C++ compiler** | Any compiler with C++23 support (e.g. Clang 16+, GCC 13+) |
| **Boost** | Required; **iostreams** and **headers** (Boost.Multiprecision with GMP). Use a recent release (e.g. 1.70+). |
| **GMP** | Required (used by Boost.Multiprecision for arbitrary-precision counts). |
| **Googletest** | Fetched automatically by CMake (no manual install) |

You must install Boost (with iostreams) and GMP on your system before configuring. Everything else is handled by CMake.

---

## Setup on your system

### 1. Install build tools and Boost

**macOS (Homebrew):**
```bash
brew install cmake ninja boost gmp
# Use system Clang (Xcode Command Line Tools) or:
brew install llvm
```

**Linux (apt):**
```bash
sudo apt update
sudo apt install cmake ninja-build build-essential libboost-all-dev libgmp-dev
# Or for a newer GCC: sudo apt install gcc-13 g++-13
```

**Windows:**  
Install [CMake](https://cmake.org/download/), [Ninja](https://github.com/ninja-build/ninja/releases), and a C++23-capable compiler (e.g. Visual Studio 2022 or LLVM/Clang). For Boost and GMP, use [vcpkg](https://vcpkg.io/) (e.g. `vcpkg install boost-iostreams gmp`) or the [official Boost binaries](https://www.boost.org/users/download/) and [GMP](https://gmplib.org/); set `BOOST_ROOT` or pass `-DBOOST_ROOT=...` when configuring if needed.

### 2. (Optional) clangd / IDE

The repo includes a [`.clangd`](.clangd) file so editors using clangd get consistent warnings and include paths. It uses **machine-specific paths** (e.g. Homebrew LLVM, Boost, and GMP on macOS). If you’re on a different OS or layout, either:

- **Remove or rename `.clangd`** and rely on `compile_commands.json` from the build (see below), or  
- **Edit `.clangd`** and update:
  - `-I...` for the project include directory (use your project root path).
  - `-isystem` paths for LLVM libc++, Boost, and GMP to match your install.

After configuring with CMake, the build directory contains `compile_commands.json`, which many IDEs and clangd can use automatically, often making `.clangd` optional.

---

## Build and test

From the project root:

```bash
# Configure (pick one preset)
cmake --preset clang-debug    # Debug build
# or
cmake --preset clang-release  # Release build

# Build (use the same preset name as above)
cmake --build out/build/clang-debug
# or
cmake --build out/build/clang-release

# Run tests
ctest --test-dir out/build/clang-debug --output-on-failure
# or
ctest --test-dir out/build/clang-release --output-on-failure
```

One-liner for a Release build and test:

```bash
cmake --preset clang-release && cmake --build out/build/clang-release && ctest --test-dir out/build/clang-release --output-on-failure
```

The main executable (after building) is:

- **Debug:** `out/build/clang-debug/cc-compression-tool-cpp`
- **Release:** `out/build/clang-release/cc-compression-tool-cpp`

---

## Project layout

```
├── CMakeLists.txt      # Main build and test definition
├── CMakePresets.json   # clang-debug / clang-release presets
├── .clangd             # Optional; adjust paths for your machine
├── include/            # Project include path (optional)
├── src/                # Application source and headers
│   ├── main.cpp
│   ├── argument/       # CLI arguments
│   ├── exceptions/     # Custom exceptions
│   ├── file_handler/   # Input file handling
│   └── frequency_table/ # Character frequency table (Huffman)
├── test/               # Googletest tests (hello, arguments, file_handler, frequency_table)
├── sample/             # Sample data (e.g. test.txt)
└── out/                # Build and install outputs (generated)
```

---

## Summary: what to do as a new user

1. **Install** CMake, Ninja, Boost (iostreams), GMP, and a C++23 compiler (see "Setup on your system").
2. **Configure, build, and test** using the commands in “Build and test” (no code changes needed if your toolchain is standard).
3. **If you use clangd/IDE:** update or disable `.clangd` so include paths and compiler flags match your system or rely on `compile_commands.json` generated in the build directory.
