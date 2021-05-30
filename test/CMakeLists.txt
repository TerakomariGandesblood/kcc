enable_testing()

set(TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${TEST_BINARY_DIR})
set(TEST_OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}/obj)
file(MAKE_DIRECTORY ${TEST_OBJ_DIR})

add_test(NAME check_executable COMMAND ${EXECUTABLE} -help)

add_test(NAME compile-testmain
         COMMAND ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/usual/testmain.c -O0 -g
                 -c -o ${TEST_OBJ_DIR}/testmain.o)

add_test(NAME compile-testmain-opt
         COMMAND ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/usual/testmain.c -O3 -g
                 -c -o ${TEST_OBJ_DIR}/testmain_opt.o)

aux_source_directory(${KCC_SOURCE_DIR}/test/usual USUAL_FILES)

foreach(USUAL_FILE ${USUAL_FILES})
  get_filename_component(USUAL_FILE_NAME ${USUAL_FILE} NAME_WE)
  if(USUAL_FILE_NAME STREQUAL "testmain")
    continue()
  endif()

  add_test(NAME compile-${USUAL_FILE_NAME}
           COMMAND ${EXECUTABLE} ${USUAL_FILE} ${TEST_OBJ_DIR}/testmain.o -O0
                   -g -o ${TEST_BINARY_DIR}/${USUAL_FILE_NAME})
  add_test(NAME run-${USUAL_FILE_NAME}
           COMMAND ${TEST_BINARY_DIR}/${USUAL_FILE_NAME})
  set_tests_properties(compile-${USUAL_FILE_NAME} PROPERTIES DEPENDS
                                                             compile-testmain)
  set_tests_properties(run-${USUAL_FILE_NAME}
                       PROPERTIES DEPENDS compile-${USUAL_FILE_NAME})

  add_test(NAME compile-${USUAL_FILE_NAME}-opt
           COMMAND ${EXECUTABLE} ${USUAL_FILE} ${TEST_OBJ_DIR}/testmain_opt.o
                   -O3 -o ${TEST_BINARY_DIR}/${USUAL_FILE_NAME}_opt)
  add_test(NAME run-${USUAL_FILE_NAME}-opt
           COMMAND ${TEST_BINARY_DIR}/${USUAL_FILE_NAME}_opt)
  set_tests_properties(compile-${USUAL_FILE_NAME}-opt
                       PROPERTIES DEPENDS compile-testmain-opt)
  set_tests_properties(run-${USUAL_FILE_NAME}-opt
                       PROPERTIES DEPENDS compile-${USUAL_FILE_NAME}-opt)

endforeach()

add_test(
  NAME "compile-8CC"
  COMMAND ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/8cc/*.c -O0 -g -std=gnu17
          -DBUILD_DIR=\"${KCC_SOURCE_DIR}/test/8cc\" -o ${TEST_BINARY_DIR}/8cc)
add_test(NAME check_8cc_executable COMMAND ${TEST_BINARY_DIR}/8cc -h)

add_test(
  NAME "compile-8CC-opt"
  COMMAND
    ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/8cc/*.c -O3 -std=gnu17
    -DBUILD_DIR=\"${KCC_SOURCE_DIR}/test/8cc\" -o ${TEST_BINARY_DIR}/8cc_opt)
add_test(NAME check_8cc_opt_executable COMMAND ${TEST_BINARY_DIR}/8cc_opt -h)

set(TEST_OBJ_DIR ${CMAKE_BINARY_DIR}/objs)
file(MAKE_DIRECTORY ${TEST_OBJ_DIR})
aux_source_directory(${KCC_SOURCE_DIR}/test/8cc 8CC_FILES)

foreach(8CC_FILE ${8CC_FILES})
  get_filename_component(8CC_FILE_NAME ${8CC_FILE} NAME_WE)

  add_test(
    NAME "compile-${8CC_FILE_NAME}_8cc"
    COMMAND
      ${TEST_BINARY_DIR}/8cc ${8CC_FILE}
      -DBUILD_DIR=\"${KCC_SOURCE_DIR}/test/8cc\" -c -o
      ${TEST_OBJ_DIR}/${8CC_FILE_NAME}.o)

  add_test(
    NAME "compile-${8CC_FILE_NAME}_8cc_opt"
    COMMAND
      ${TEST_BINARY_DIR}/8cc_opt ${8CC_FILE}
      -DBUILD_DIR=\"${KCC_SOURCE_DIR}/test/8cc\" -c -o
      ${TEST_OBJ_DIR}/${8CC_FILE_NAME}.o)
endforeach()

add_test(
  NAME "compile-LUA"
  COMMAND
    ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/lua/*.c -O0 -g -std=gnu17
    -DLUA_USER_H=\"ltests.h\" -DLUA_USE_LINUX -DLUA_COMPAT_5_2 -ldl -lreadline
    -lm -o ${TEST_BINARY_DIR}/lua)
add_test(NAME check_lua_executable COMMAND ${TEST_BINARY_DIR}/lua -v)

add_test(
  NAME "compile-LUA-opt"
  COMMAND
    ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/lua/*.c -O3 -std=gnu17
    -DLUA_USER_H=\"ltests.h\" -DLUA_USE_LINUX -DLUA_COMPAT_5_2 -ldl -lreadline
    -lm -o ${TEST_BINARY_DIR}/lua_opt)
add_test(NAME check_lua_opt_executable COMMAND ${TEST_BINARY_DIR}/lua_opt -v)

add_test(
  NAME lua_test
  COMMAND ${TEST_BINARY_DIR}/lua ${KCC_SOURCE_DIR}/test/lua/testes/all.lua
  WORKING_DIRECTORY ${KCC_SOURCE_DIR}/test/lua/testes)

add_test(
  NAME lua_test_opt
  COMMAND ${TEST_BINARY_DIR}/lua_opt ${KCC_SOURCE_DIR}/test/lua/testes/all.lua
  WORKING_DIRECTORY ${KCC_SOURCE_DIR}/test/lua/testes)

add_test(
  NAME "compile-SQLITE"
  COMMAND
    ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/sqlite/shell.c
    ${KCC_SOURCE_DIR}/test/sqlite/sqlite3.c -o ${TEST_BINARY_DIR}/sqlite -O0 -g
    -lpthread -ldl -lm -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_DQS=0
    -DSQLITE_ENABLE_DBSTAT_VTAB -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_GEOPOLY
    -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RBU -DSQLITE_ENABLE_RTREE
    -DSQLITE_LIKE_DOESNT_MATCH_BLOBS -DSQLITE_MAX_EXPR_DEPTH=0
    -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED -DSQLITE_USE_ALLOCA
    -DSQLITE_ENABLE_MEMSYS5)
add_test(NAME check_sqlite_executable COMMAND ${TEST_BINARY_DIR}/sqlite
                                              -version)

add_test(
  NAME "compile-SQLITE-opt"
  COMMAND
    ${EXECUTABLE} ${KCC_SOURCE_DIR}/test/sqlite/shell.c
    ${KCC_SOURCE_DIR}/test/sqlite/sqlite3.c -o ${TEST_BINARY_DIR}/sqlite_opt
    -O3 -lpthread -ldl -lm -DNDEBUG -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_DQS=0
    -DSQLITE_ENABLE_DBSTAT_VTAB -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_GEOPOLY
    -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RBU -DSQLITE_ENABLE_RTREE
    -DSQLITE_LIKE_DOESNT_MATCH_BLOBS -DSQLITE_MAX_EXPR_DEPTH=0
    -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED -DSQLITE_USE_ALLOCA
    -DSQLITE_ENABLE_MEMSYS5)
add_test(NAME check_sqlite_opt_executable COMMAND ${TEST_BINARY_DIR}/sqlite_opt
                                                  -version)

include(Coverage)
include(Valgrind)
