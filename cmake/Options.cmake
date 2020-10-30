option(KCC_BUILD_TESTS "Build tests" ON)

option(KCC_FORMAT "Format code using clang-format and cmake-format" OFF)
option(KCC_CLANG_TIDY "Analyze code with clang-tidy" OFF)

option(KCC_USE_LIBCXX "Use libc++" OFF)

include(CMakeDependentOption)
cmake_dependent_option(
  KCC_BUILD_COVERAGE "Build tests with coverage information" OFF
  "BUILD_TESTING;KCC_BUILD_TESTS" OFF)
cmake_dependent_option(KCC_VALGRIND "Execute tests with valgrind" OFF
                       "BUILD_TESTING;KCC_BUILD_TESTS" OFF)

set(KCC_SANITIZER
    ""
    CACHE
      STRING
      "Build with a sanitizer. Options are: Address, Thread, Memory, Undefined and Address;Undefined"
)
