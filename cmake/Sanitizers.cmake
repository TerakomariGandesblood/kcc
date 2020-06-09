if(SANITIZER STREQUAL "None")

else()
  include(Utility)

  if(SANITIZER STREQUAL "Address")
    message(STATUS "Building with Address and Undefined Sanitizer")
    append("-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer"
           CMAKE_CXX_FLAGS)
  elseif(SANITIZER STREQUAL "Memory")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      message(
        STATUS
          "Building with Undefined Sanitizer, GCC does not support Memory Sanitizer"
      )
      append("-fsanitize=undefined" CMAKE_CXX_FLAGS)
    else()
      message(STATUS "Building with Memory and Undefined Sanitizer")
      append(
        "-fsanitize=memory -fsanitize-memory-track-origins -fsanitize=undefined -fno-omit-frame-pointer -fPIE"
        CMAKE_CXX_FLAGS)
    endif()
  elseif(SANITIZER STREQUAL "Thread")
    message(STATUS "Building with Thread and Undefined Sanitizer")
    append("-fsanitize=thread -fsanitize=undefined" CMAKE_CXX_FLAGS)
  else()
    message(FATAL_ERROR "Sanitizer is not supported")
  endif()
endif()
