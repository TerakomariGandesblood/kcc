# https://github.com/google/benchmark/blob/master/cmake/AddCXXCompilerFlag.cmake
function(mangle_compiler_flag FLAG OUTPUT)
  string(TOUPPER "HAVE_FLAG_${FLAG}" SANITIZED_FLAG)
  string(REPLACE "+" "X" SANITIZED_FLAG ${SANITIZED_FLAG})
  string(REGEX REPLACE "[^A-Za-z_0-9]" "_" SANITIZED_FLAG ${SANITIZED_FLAG})
  string(REGEX REPLACE "_+" "_" SANITIZED_FLAG ${SANITIZED_FLAG})

  set(${OUTPUT}
      ${SANITIZED_FLAG}
      PARENT_SCOPE)
endfunction()

include(CheckCXXCompilerFlag)

function(add_cxx_compiler_flag FLAG)
  mangle_compiler_flag(${FLAG} MANGLED_FLAG)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  check_cxx_compiler_flag(${FLAG} ${MANGLED_FLAG}_CXX)
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})

  if(${MANGLED_FLAG}_CXX)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${FLAG}"
        PARENT_SCOPE)
  else()
    message(
      FATAL_ERROR "Required flag '${FLAG}' is not supported by the compiler")
  endif()
endfunction()
