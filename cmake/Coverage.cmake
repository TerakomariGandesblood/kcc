if(KCC_BUILD_COVERAGE)
  if(CMAKE_COMPILER_IS_GNUCXX)
    message(
      STATUS "Build with coverage information, use lcov to generate report")

    # https://github.com/nlohmann/json/blob/develop/test/CMakeLists.txt
    get_filename_component(COMPILER_PATH ${CMAKE_CXX_COMPILER} PATH)
    string(REGEX MATCH "^[0-9]+" GCC_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    find_program(
      GCOV_EXECUTABLE
      NAMES gcov-${GCC_VERSION} gcov
      HINTS ${COMPILER_PATH})
    if(NOT GCOV_EXECUTABLE)
      message(FATAL_ERROR "Can not find gcov")
    endif()

    find_program(LCOV_EXECUTABLE lcov)
    if(NOT LCOV_EXECUTABLE)
      message(FATAL_ERROR "Can not find lcov")
    endif()

    find_program(GENHTML_EXECUTABLE genhtml)
    if(NOT GENHTML_EXECUTABLE)
      message(FATAL_ERROR "Can not find genhtml")
    endif()

    # https://github.com/linux-test-project/lcov
    # https://github.com/nlohmann/json/blob/develop/test/CMakeLists.txt
    add_custom_target(
      coverage
      COMMAND ${LCOV_EXECUTABLE} -d ${KCC_BINARY_DIR} -z
      COMMAND ctest --output-on-failure
      COMMAND
        ${LCOV_EXECUTABLE} -d ${KCC_BINARY_DIR} --include
        '${KCC_SOURCE_DIR}/src/*.cpp' --include '${KCC_SOURCE_DIR}/include/*.h'
        --gcov-tool ${GCOV_EXECUTABLE} -c -o lcov.info --rc
        lcov_branch_coverage=1
      COMMAND ${GENHTML_EXECUTABLE} lcov.info -o coverage -s --title
              "${PROJECT_NAME}" --legend --demangle-cpp --branch-coverage
      WORKING_DIRECTORY ${KCC_BINARY_DIR}
      DEPENDS ${TEST_EXECUTABLE}
      COMMENT "Generate HTML report: ${KCC_BINARY_DIR}/coverage/index.html")
  else()
    message(FATAL_ERROR "Only use lcov to generate coverage information")
  endif()
endif()
