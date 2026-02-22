# cc-compression-tool-cpp

Huffman encoder/decoder built in C++ (C++23), based on [Coding Challenges](https://codingchallenges.fyi/) by John Crickett.

---

## Dependencies

| Dependency | Version / notes |
|------------|------------------|
| **CMake** | 3.10 or later |
| **Build system** | Ninja (used by presets) |
| **C++ compiler** | Any compiler with C++23 support (e.g. Clang 16+, GCC 13+) |
| **Googletest** | Fetched automatically by CMake (no manual install) |

Nothing else is required to configure, build, or run tests. The project does not use Boost or other external libraries in the build.

---

## Setup on your system

### 1. Install build tools

**macOS (Homebrew):**
```bash
brew install cmake ninja
# Use system Clang (Xcode Command Line Tools) or:
brew install llvm
```

**Linux (apt):**
```bash
sudo apt update
sudo apt install cmake ninja-build build-essential
# Or for a newer GCC: sudo apt install gcc-13 g++-13
```

**Windows:**  
Install [CMake](https://cmake.org/download/) and [Ninja](https://github.com/ninja-build/ninja/releases), and a C++23-capable compiler (e.g. Visual Studio 2022 or LLVM/Clang).

### 2. (Optional) clangd / IDE

The repo includes a [`.clangd`](.clangd) file so editors using clangd get consistent warnings and include paths. It currently uses **machine-specific paths** (e.g. Homebrew LLVM and Boost on macOS). If you’re on a different OS or layout, either:

- **Remove or rename `.clangd`** and rely on `compile_commands.json` from the build (see below), or  
- **Edit `.clangd`** and update:
  - `-I...` for the project `include` directory (use your project root path).
  - `-isystem` paths for LLVM libc++ and (if you use it) Boost to match your install.

After configuring with CMake, the build directory will contain `compile_commands.json`, which many IDEs and clangd can use automatically, often making `.clangd` optional.

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
├── include/            # Project headers
├── src/                # Application source (e.g. main.cpp)
├── test/               # Googletest tests
├── sample/             # Sample data (e.g. test.txt)
└── out/                # Build and install outputs (generated)
```

---

## Summary: what to change for a new user

1. **Install** CMake, Ninja, and a C++23 compiler (see above).
2. **Build and test** using the commands in “Build and test” (no edits required if your toolchain is standard).
3. **If you use clangd/IDE:** update or disable `.clangd` so include paths and compiler flags match your system (or rely on `compile_commands.json` from the build).
