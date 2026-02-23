# ┌───────────────────────────────────────────────────────────────┐
# │ Makefile for rheo project – CMake wrapper                     │
# │                                                               │
# │ Common commands:                                              │
# │   make          → configure + build                           │
# │   make run      → build + run the executable                  │
# │   make clean    → remove build artifacts                      │
# │   make distclean→ full wipe of build/ folder                  │
# │   make help     → show this help                              │
# └───────────────────────────────────────────────────────────────┘

BUILD_DIR     := build
EXECUTABLE    := rheo
CMAKE_FLAGS   ?=
CTEST_FLAGS   ?= --output-on-failure --progress

.DEFAULT_GOAL := help

# ─────────────────────────────────────────────────────────────────
# Main targets
# ─────────────────────────────────────────────────────────────────

.PHONY: all
all: build

.PHONY: configure
configure:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS)

.PHONY: build
build: configure
	cmake --build $(BUILD_DIR) --parallel $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

.PHONY: run
run: build
	@$(BUILD_DIR)/$(EXECUTABLE)

.PHONY: clean
clean:
	cmake --build $(BUILD_DIR) --target clean -- -j $(shell nproc 2>/dev/null || echo 4) 2>/dev/null || true
	@if [ -d "$(BUILD_DIR)" ]; then rm -rf $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/CMakeFiles; fi

.PHONY: distclean
distclean:
	rm -rf $(BUILD_DIR)

# ─────────────────────────────────────────────────────────────────
# Optional / future extensions
# ─────────────────────────────────────────────────────────────────

.PHONY: test
test: build
	cd $(BUILD_DIR) && ctest $(CTEST_FLAGS)

.PHONY: format
format:
	find source -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cxx" -o -name "*.hxx" \) \
	  -exec clang-format -i --style=file {} +

.PHONY: tidy
tidy: configure
	run-clang-tidy -p $(BUILD_DIR) -quiet -header-filter='.*' source/*.cpp || true

# ─────────────────────────────────────────────────────────────────
# Help
# ─────────────────────────────────────────────────────────────────

.PHONY: help
help:
	@echo ""
	@echo "Makefile help for project 'rheo'"
	@echo "────────────────────────────────"
	@echo "  make              : Configure (if needed) + build"
	@echo "  make run          : Build + run ./$(EXECUTABLE)"
	@echo "  make clean        : Clean most build artifacts"
	@echo "  make distclean    : Delete entire build/ folder"
	@echo ""
	@echo "  make test         : Run tests (if you add them later)"
	@echo "  make format       : Run clang-format on source files"
	@echo "  make tidy         : Run clang-tidy (needs run-clang-tidy)"
	@echo ""
	@echo "Extra options:"
	@echo "  make CMAKE_FLAGS=\"-DCMAKE_BUILD_TYPE=Debug\"   → example"
	@echo "  make CMAKE_FLAGS=\"-G Ninja\"                   → faster builds"
	@echo ""
