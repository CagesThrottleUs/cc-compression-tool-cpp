cmake_minimum_required(VERSION 3.10.0)
project(cc-compression-tool-cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set src files
set(SRC_FILES
  src/main.cpp
)

add_executable(cc-compression-tool-cpp ${SRC_FILES})

target_include_directories(cc-compression-tool-cpp PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add googletest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/e9907112b47255d50b4d343e7e2160bce8dc85d1.zip
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test files
set(TEST_FILES
  test/hello_test.cpp
)

# Disable clang-tidy for gtest targets
set_target_properties(gtest gtest_main PROPERTIES
    SKIP_CLANG_TIDY TRUE
)


add_executable(cc_compression_tool_test ${TEST_FILES})
target_link_libraries(cc_compression_tool_test PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(cc_compression_tool_test)