if(SANITIZER STREQUAL "Address")
  message(STATUS "Building with AddressSanitizer and UndefinedSanitizer")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer"
  )
elseif(SANITIZER STREQUAL "Memory")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(
      STATUS
        "Building with UndefinedSanitizer, GCC does not support MemorySanitizer"
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  else()
    message(STATUS "Building with MemorySanitizer and UndefinedSanitizer")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fsanitize=undefined -fno-omit-frame-pointer -fPIE"
    )
  endif()
elseif(SANITIZER STREQUAL "Thread")
  message(STATUS "Building with ThreadSanitizer and UndefinedSanitizer")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -fsanitize=thread -fsanitize=undefined")
elseif(SANITIZER STREQUAL "None")

else()
  message(FATAL_ERROR "The Sanitizer is not supported")
endif()
