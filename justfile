# ┌───────────────────────────────────────────────────────────────┐
# │ justfile for rheo project – CMake wrapper                     │
# │                                                               │
# │ Common commands:                                              │
# │   just            → show this help                            │
# │   just build      → configure + build                         │
# │   just run        → build + run the executable                │
# │   just clean      → remove build artifacts                    │
# │   just distclean  → full wipe of build/ folder                │
# └───────────────────────────────────────────────────────────────┘

build_dir := "build"
executable := "rheo"
cmake_flags := ""
ctest_flags := "--output-on-failure --progress"

_ncpu := `nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4`

# Show available recipes
default:
    @just --list

# Configure the project with CMake
configure:
    mkdir -p {{build_dir}}
    cd {{build_dir}} && cmake .. {{cmake_flags}}

# Configure build
build: configure
    cmake --build {{build_dir}} --parallel {{_ncpu}}

# Build + run the executable
run: build
    {{build_dir}}/{{executable}}

# Clean most build artifacts
clean:
    cmake --build {{build_dir}} --target clean -- -j {{_ncpu}} 2>/dev/null || true
    @if [ -d "{{build_dir}}" ]; then rm -rf {{build_dir}}/CMakeCache.txt {{build_dir}}/CMakeFiles; fi

# Delete entire build/ folder
distclean:
    rm -rf {{build_dir}}

# Run tests (ctest)
test: build
    cd {{build_dir}} && ctest {{ctest_flags}}

# Run clang-format on all source files
format:
    find source -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cxx" -o -name "*.hxx" \) \
      -exec clang-format -i --style=file {} +

# Run clang-tidy 
tidy: configure
    run-clang-tidy -p {{build_dir}} -quiet -header-filter='.*' source/*.cpp || true
