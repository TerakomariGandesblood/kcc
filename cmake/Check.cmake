if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
  message(STATUS "x86_64 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "AMD64")
  message(STATUS "AMD64 architecture detected")
else()
  message(FATAL_ERROR "The architecture does not support: "
                      ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Linux and GCC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.2.0)
      message(FATAL_ERROR "GCC version must be at least 10.2.0: "
                          ${CMAKE_CXX_COMPILER_VERSION})
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Linux and Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0.0)
      message(FATAL_ERROR "Clang version must be at least 11.0.0: "
                          ${CMAKE_CXX_COMPILER_VERSION})
    endif()
  endif()
else()
  message(FATAL_ERROR "The system and compiler combination does not support: "
                      ${CMAKE_SYSTEM_NAME} " " ${CMAKE_CXX_COMPILER_ID})
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose Release, Debug, MinSizeRel or RelWithDebInfo" FORCE)
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
