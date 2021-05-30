if(KCC_FORMAT)
  message(STATUS "Format code using clang-format and cmake-format")

  find_program(CLANG_FORMAT_EXECUTABLE clang-format)
  if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR "Can not find clang-format")
  endif()

  find_program(CMAKE_FORMAT_EXECUTABLE cmake-format)
  if(NOT CMAKE_FORMAT_EXECUTABLE)
    message(FATAL_ERROR "Can not find cmake-format")
  endif()

  file(
    GLOB_RECURSE
    CLANG_FORMAT_SRC
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/example/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tool/*.cpp")

  file(
    GLOB_RECURSE
    CMAKE_FORMAT_SRC
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/tool/CMakeLists.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

  add_custom_target(
    format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${CLANG_FORMAT_SRC}
    COMMAND ${CMAKE_FORMAT_EXECUTABLE} -i ${CMAKE_FORMAT_SRC}
    COMMENT "Start formatting code")
endif()
