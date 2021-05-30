# https://github.com/google/benchmark/blob/master/cmake/AddCXXCompilerFlag.cmake
include(CheckCXXCompilerFlag)

function(mangle_compiler_flag FLAG OUTPUT)
  string(TOUPPER "HAVE_CXX_FLAG_${FLAG}" SANITIZED_FLAG)
  string(REPLACE "+" "X" SANITIZED_FLAG ${SANITIZED_FLAG})
  string(REGEX REPLACE "[^A-Za-z_0-9]" "_" SANITIZED_FLAG ${SANITIZED_FLAG})
  string(REGEX REPLACE "_+" "_" SANITIZED_FLAG ${SANITIZED_FLAG})

  # https://stackoverflow.com/questions/22487215/return-a-list-from-the-function-using-out-parameter
  set(${OUTPUT}
      ${SANITIZED_FLAG}
      PARENT_SCOPE)
endfunction()

function(add_cxx_compiler_flag FLAG)
  mangle_compiler_flag(${FLAG} MANGLED_FLAG)

  # https://cmake.org/cmake/help/latest/module/CheckCXXCompilerFlag.html
  # https://cmake.org/cmake/help/latest/module/CheckCXXSourceCompiles.html
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  check_cxx_compiler_flag(${FLAG} ${MANGLED_FLAG})
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})

  if(${MANGLED_FLAG})
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${FLAG}"
        PARENT_SCOPE)
  else()
    message(
      FATAL_ERROR "Required flag '${FLAG}' is not supported by the compiler")
  endif()
endfunction()
