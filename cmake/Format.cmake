find_program(CLANG_FORMAT_PATH NAMES clang-format)

if(CLANG_FORMAT_PATH)
  message(STATUS "clang-format found")
  add_custom_target(clang_format COMMAND ${CLANG_FORMAT_PATH} -i -verbose
                                         ${CLANG_FORMAT_SOURCES})
  add_dependencies(${PROGRAM_NAME} clang_format)
else()
  message(STATUS "clang-format not found")
endif()

find_program(CMAKE_FORMAT_PATH NAMES cmake-format)

if(CMAKE_FORMAT_PATH)
  message(STATUS "cmake-format found")
  add_custom_target(cmake_format COMMAND ${CMAKE_FORMAT_PATH} -i
                                         ${CMAKE_FORMAT_SOURCES})
  add_dependencies(${PROGRAM_NAME} cmake_format)
else()
  message(STATUS "cmake-format not found")
endif()
