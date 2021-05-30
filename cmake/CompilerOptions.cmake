include(AddCXXCompilerFlag)

# FIXME
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------------------
# lld
# ---------------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  execute_process(
    COMMAND ld.lld --version
    OUTPUT_VARIABLE LLD_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Linker: ${LLD_VERSION}")

  add_link_options("-fuse-ld=lld")
else()
  execute_process(
    COMMAND ${CMAKE_LINKER} --version
    OUTPUT_VARIABLE LINKER_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\n" ";" LINKER_VERSION ${LINKER_VERSION})
  list(GET LINKER_VERSION 0 LINKER_VERSION)

  message(STATUS "Linker: ${LINKER_VERSION}")
endif()

# ---------------------------------------------------------------------------------------
# Static link
# ---------------------------------------------------------------------------------------
add_link_options("-static-libstdc++")
add_link_options("-static-libgcc")

# ---------------------------------------------------------------------------------------
# Warning
# ---------------------------------------------------------------------------------------
add_cxx_compiler_flag("-Wall")
add_cxx_compiler_flag("-Wextra")
add_cxx_compiler_flag("-Wpedantic")
add_cxx_compiler_flag("-Werror")

# FIXME
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_cxx_compiler_flag("-Wno-unused-parameter")
else()
  add_cxx_compiler_flag("-Wno-unused-parameter")
  add_cxx_compiler_flag("-Wno-mismatched-new-delete")
endif()

# ---------------------------------------------------------------------------------------
# Link time optimization
# ---------------------------------------------------------------------------------------
# https://github.com/ninja-build/ninja/blob/master/CMakeLists.txt
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT LTO_SUPPORTED
    OUTPUT ERROR
    LANGUAGES CXX)

  if(LTO_SUPPORTED)
    message(STATUS "Link time optimization: enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(FATAL_ERROR "Link time optimization not supported: ${ERROR}")
  endif()
else()
  message(STATUS "Link time optimization: disable")
endif()

# ---------------------------------------------------------------------------------------
# Sanitizer
# ---------------------------------------------------------------------------------------
if(KCC_SANITIZER)
  message(STATUS "Build with AddressSanitizer and UndefinedSanitizer")
  add_cxx_compiler_flag("-fno-omit-frame-pointer")

  add_cxx_compiler_flag("-fsanitize=address")
  add_cxx_compiler_flag("-fsanitize-address-use-after-scope")
  add_cxx_compiler_flag("-fno-optimize-sibling-calls")

  add_cxx_compiler_flag("-fsanitize=undefined")
  add_cxx_compiler_flag("-fno-sanitize-recover=all")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_cxx_compiler_flag("-fsanitize=float-divide-by-zero")
    add_cxx_compiler_flag("-fsanitize=unsigned-integer-overflow")
    add_cxx_compiler_flag("-fsanitize=implicit-conversion")
    add_cxx_compiler_flag("-fsanitize=local-bounds")
    add_cxx_compiler_flag("-fsanitize=nullability")
    add_cxx_compiler_flag("-fsanitize-recover=unsigned-integer-overflow")
  endif()
endif()

# ---------------------------------------------------------------------------------------
# Coverage
# ---------------------------------------------------------------------------------------
if(KCC_BUILD_COVERAGE)
  add_cxx_compiler_flag("--coverage")
endif()
