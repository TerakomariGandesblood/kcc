if(KCC_VALGRIND)
  message(STATUS "Execute test with valgrind")

  find_program(VALGRIND_EXECUTABLE valgrind)

  if(NOT VALGRIND_EXECUTABLE)
    message(FATAL_ERROR "Can not find valgrind")
  endif()

  add_test(
    NAME ${TEST_EXECUTABLE}-valgrind
    COMMAND
      ${VALGRIND_EXECUTABLE} --leak-check=full --show-leak-kinds=all
      --leak-resolution=med --track-origins=yes --vgdb=no --tool=memcheck
      --gen-suppressions=all --error-exitcode=1 ./${TEST_EXECUTABLE}
    WORKING_DIRECTORY ${KCC_BINARY_DIR}/test)
endif()
