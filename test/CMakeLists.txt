# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(rheoTests LANGUAGES CXX)

# ---- Tests ----

add_executable(rheo_test source/rheo_test.cpp)
target_link_libraries(rheo_test PRIVATE rheo_lib)
target_compile_features(rheo_test PRIVATE cxx_std_23)

add_test(NAME rheo_test COMMAND rheo_test)

# ---- End-of-file commands ----

add_folders(Test)
