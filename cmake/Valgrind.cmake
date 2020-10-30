if(KCC_VALGRIND)
  # TODO
  message(STATUS "Execute tests with valgrind")

  find_program(VALGRIND_EXECUTABLE valgrind)

  if(NOT VALGRIND_EXECUTABLE)
    message(FATAL_ERROR "Can not find valgrind")
  endif()

  add_test(
    NAME ${TESTS_EXECUTABLE}-valgrind
    COMMAND ${VALGRIND_EXECUTABLE} --error-exitcode=1 --track-origins=yes
            --gen-suppressions=all --leak-check=full ./${TESTS_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
