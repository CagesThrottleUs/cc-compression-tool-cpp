cmake_minimum_required(VERSION 3.15.0)
project(cc-compression-tool-cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------- Find Packages -----------------------------

# 1. Boost (iostreams is compiled, others are headers)
find_package(Boost REQUIRED COMPONENTS iostreams CONFIG)

# 2. GMP (Search for headers and library)
find_path(GMP_INCLUDE_DIR NAMES gmp.h)
find_library(GMP_LIBRARY NAMES gmp)

if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY)
    message(FATAL_ERROR "GMP not found. Run 'brew install gmp' on macOS.")
endif()

# Create a "Formal" CMake Target for GMP (Cleaner than using variables)
add_library(GMP::GMP UNKNOWN IMPORTED)
set_target_properties(GMP::GMP PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
    IMPORTED_LOCATION "${GMP_LIBRARY}"
)

# ----------------------------- Main Executable -----------------------------

set(SRC_FILES
  src/main.cpp
  src/exceptions/exceptions.cpp
  src/argument/arguments.cpp
  src/file_handler/input_file.cpp
  src/frequency_table/frequency_table.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link everything to the main app
target_link_libraries(${PROJECT_NAME} PRIVATE
  Boost::iostreams
  Boost::headers
  GMP::GMP
)

# Crucial: Tell Boost to use GMP
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_MULTIPRECISION_USE_GMP)

# ----------------------------- Testing -----------------------------

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/e9907112b47255d50b4d343e7e2160bce8dc85d1.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(cc_compression_tool_test 
    test/hello_test.cpp
    test/arguments_test.cpp
    test/file_handler_test.cpp
    src/argument/arguments.cpp
    src/exceptions/exceptions.cpp
    src/file_handler/input_file.cpp
    src/frequency_table/frequency_table.cpp # Added this so your tests can find the freq logic
)

target_include_directories(cc_compression_tool_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cc_compression_tool_test PRIVATE
  GTest::gtest_main
  Boost::iostreams
  Boost::headers
  GMP::GMP
)

# Tell Boost to use GMP in tests too
target_compile_definitions(cc_compression_tool_test PRIVATE BOOST_MULTIPRECISION_USE_GMP)

include(GoogleTest)
gtest_discover_tests(cc_compression_tool_test)