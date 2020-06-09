if(NOT COVERAGE)
  if((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES
                                            "RelWithDebInfo"))
    set(COVERAGE ON)
  else()
    set(COVERAGE OFF)
  endif()
endif()

if(${COVERAGE})
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Building with lcov Code Coverage Tools")

    include(Utility)
    append("--coverage" CMAKE_CXX_FLAGS)

    find_program(LCOV_PATH NAMES lcov)
    find_program(GENHTML_PATH NAMES genhtml)

    if(LCOV_PATH AND GENHTML_PATH)
      # TODO 考虑使用 llvm-cov
      add_custom_target(
        coverage
        # COMMAND ${LCOV_PATH} --directory . --zerocounters
        # COMMAND ${PROGRAM_NAME} ${COVERAGE_RUN_ARGS}
        COMMAND ${LCOV_PATH} --no-external -b ${CMAKE_SOURCE_DIR} --directory .
                --capture --output-file coverage.info
        COMMAND ${GENHTML_PATH} -o coverage coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${PROGRAM_NAME})
    else()
      message(FATAL_ERROR "lcov or genhtml not found")
    endif()
  else()
    message(STATUS "Building with llvm Code Coverage Tools")
    append("-fprofile-instr-generate -fcoverage-mapping" CMAKE_CXX_FLAGS)

    find_program(LLVM_PROFDATA_PATH NAMES llvm-profdata)
    find_program(LLVM_COV_PATH NAMES llvm-cov)

    if(LLVM_PROFDATA_PATH AND LLVM_COV_PATH)
      add_custom_target(
        coverage
        COMMAND ${PROGRAM_NAME} ${COVERAGE_RUN_ARGS}
        COMMAND ${LLVM_PROFDATA_PATH} merge -sparse -o ${PROGRAM_NAME}.profdata
                default.profraw
        COMMAND
          ${LLVM_COV_PATH} show ${PROGRAM_NAME}
          -instr-profile=${PROGRAM_NAME}.profdata -format=html
          -output-dir=coverage
        COMMAND
          ${LLVM_COV_PATH} export ${PROGRAM_NAME}
          -instr-profile=${PROGRAM_NAME}.profdata -format=lcov > coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${PROGRAM_NAME})
    else()
      message(FATAL_ERROR "llvm-profdata or llvm-cov not found")
    endif()
  endif()
endif()
