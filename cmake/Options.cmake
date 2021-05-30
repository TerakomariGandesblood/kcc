option(KCC_BUILD_TEST "Build test" OFF)

option(KCC_FORMAT "Format code using clang-format and cmake-format" OFF)
option(KCC_CLANG_TIDY "Analyze code with clang-tidy" OFF)
option(KCC_SANITIZER "Build with AddressSanitizer and UndefinedSanitizer" OFF)

include(CMakeDependentOption)
cmake_dependent_option(KCC_BUILD_COVERAGE "Build with coverage information" OFF
                       "BUILD_TESTING;KCC_BUILD_TEST" OFF)
cmake_dependent_option(KCC_VALGRIND "Execute test with valgrind" OFF
                       "BUILD_TESTING;KCC_BUILD_TEST" OFF)
