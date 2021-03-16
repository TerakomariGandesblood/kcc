if(NOT DEFINED KCC_MASTER_PROJECT)
  if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(KCC_MASTER_PROJECT ON)
  else()
    set(KCC_MASTER_PROJECT OFF)
  endif()
endif()

option(KCC_BUILD_STATIC "Build static library" ON)
option(KCC_BUILD_SHARED "Build shared library" ON)

option(KCC_BUILD_ALL
       "Build all executable, tests, benchmarks, documentations and coverage"
       OFF)
option(KCC_BUILD_EXECUTABLE "Build executable" ${KCC_MASTER_PROJECT})
option(KCC_BUILD_TESTS "Build tests" OFF)
option(KCC_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(KCC_BUILD_DOCS "Build documentations" OFF)

option(KCC_FORMAT "Format code using clang-format and cmake-format" OFF)
option(KCC_CLANG_TIDY "Analyze code with clang-tidy" OFF)

option(KCC_INSTALL "Generate the install target" ${KCC_MASTER_PROJECT})

option(KCC_USE_LIBCXX "Use libc++" OFF)

include(CMakeDependentOption)
cmake_dependent_option(
  KCC_BUILD_COVERAGE "Build tests with coverage information" OFF
  "BUILD_TESTING;KCC_BUILD_TESTS OR KCC_BUILD_ALL" OFF)
cmake_dependent_option(KCC_VALGRIND "Execute tests with valgrind" OFF
                       "BUILD_TESTING;KCC_BUILD_TESTS OR KCC_BUILD_ALL" OFF)

set(KCC_SANITIZER
    ""
    CACHE
      STRING
      "Build with a sanitizer. Options are: Address, Thread, Memory, Undefined and Address;Undefined"
)
