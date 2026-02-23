cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/prelude.cmake)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(
    rheo
    VERSION 0.1.0
    DESCRIPTION "Rheo is a modern systems-level programming language designed with a single guiding principle: maximum power, minimum ceremony."
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES C CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Find MLIR/LLVM ----
find_package(MLIR REQUIRED CONFIG)

message(STATUS "Using MLIR: ${MLIR_DIR}")
message(STATUS "Using LLVM: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(TableGen)
include(AddLLVM)
include(AddMLIR)

# ---- Declare library ----
add_library(
    rheo_lib OBJECT
    source/lib.cpp
    # source/Diagnostics/SourceManager.cpp
)

target_include_directories(
  rheo_lib
  ${warning_guard}
  PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    ${LLVM_INCLUDE_DIRS}
    ${MLIR_INCLUDE_DIRS}
)

target_compile_features(rheo_lib PUBLIC cxx_std_23)

target_link_libraries(rheo_lib
    PUBLIC
    MLIRIR
    MLIRParser
    MLIRSupport
    LLVMSupport
    LLVM
)

# ---- Declare executable ----
add_executable(rheo_exe source/main.cpp)
add_executable(rheo::exe ALIAS rheo_exe)
set_property(TARGET rheo_exe PROPERTY OUTPUT_NAME rheo)
target_compile_features(rheo_exe PRIVATE cxx_std_23)
target_link_libraries(rheo_exe PRIVATE rheo_lib)

# ---- Install rules ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----
if(NOT rheo_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of rheo"
  )
endif()

include(cmake/dev-mode.cmake)
