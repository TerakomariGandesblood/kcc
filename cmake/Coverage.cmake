if(${COVERAGE})
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Building with LCOV code coverage tool")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

    find_program(LCOV_PATH NAMES lcov)
    find_program(GENHTML_PATH NAMES genhtml)

    if(NOT LCOV_PATH)
      message(FATAL_ERROR "lcov not found")
    endif()

    if(NOT GENHTML_PATH)
      message(FATAL_ERROR "genhtml not found")
    endif()

    add_custom_target(
      coverage
      COMMAND ${LCOV_PATH} -d . -z
      COMMAND ${TEST_PROGRAM_NAME} ${RUN_ARGS}
      COMMAND ${LCOV_PATH} --include '${CMAKE_SOURCE_DIR}/*' -d . -c -o
              coverage.info
      COMMAND ${GENHTML_PATH} coverage.info -o coverage
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${TEST_PROGRAM_NAME})
  else()
    message(STATUS "Building with llvm-cov code coverage tool")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")

    find_program(LLVM_PROFDATA_PATH NAMES llvm-profdata)
    find_program(LLVM_COV_PATH NAMES llvm-cov)

    if(NOT LLVM_PROFDATA_PATH)
      message(FATAL_ERROR "llvm-profdata not found")
    endif()

    if(NOT LLVM_COV_PATH)
      message(FATAL_ERROR "llvm-cov not found")
    endif()

    add_custom_target(
      coverage
      COMMAND ${TEST_PROGRAM_NAME} ${RUN_ARGS}
      COMMAND ${LLVM_PROFDATA_PATH} merge -sparse -o
              ${TEST_PROGRAM_NAME}.profdata default.profraw
      COMMAND
        ${LLVM_COV_PATH} show ${TEST_PROGRAM_NAME}
        -instr-profile=${TEST_PROGRAM_NAME}.profdata -format=html
        -output-dir=coverage
      COMMAND
        ${LLVM_COV_PATH} export ${TEST_PROGRAM_NAME}
        -instr-profile=${TEST_PROGRAM_NAME}.profdata -format=lcov >
        coverage.info
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${TEST_PROGRAM_NAME})
  endif()
endif()
