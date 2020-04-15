cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build" FORCE)
endif()

project(
  kcc
  VERSION 0.4
  LANGUAGES CXX)

file(GLOB cppsrc src/*.cpp)
file(GLOB cppheader src/*.h)
file(GLOB cmakesrc CMakeLists.txt cmake/*.cmake)

if((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES
                                          "RelWithDebInfo"))
  # Address Thread Undefined None
  set(sanitizer "Undefined")
else()
  set(sanitizer "None")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(check)
include(sanitizers)
include(clang-tidy)
include(compiler-options)

set(CMAKE_AUTOMOC ON)

find_package(fmt REQUIRED)
find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
find_package(Boost REQUIRED COMPONENTS locale)
find_package(Qt5 REQUIRED COMPONENTS Core)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(-DKCC_VERSION="${PROJECT_VERSION}" -DFMT_STRING_ALIAS
                ${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} ${cppsrc})

if((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES
                                          "RelWithDebInfo"))
  add_definitions(-DDEV)
  include(format)
endif()

target_link_libraries(
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  Qt5::Core
  fmt::fmt
  clangLex
  clangDriver
  clangFrontend
  clangBasic
  clangSerialization
  LLVM
  lldELF)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

add_custom_target(uninstall COMMAND rm
                                    ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME})

enable_testing()

set(TEST_BINARY_DIR ${CMAKE_BINARY_DIR}/tests)
file(MAKE_DIRECTORY ${TEST_BINARY_DIR})

set(KCC_EXECUTABLE ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
add_test(NAME check_kcc_executable COMMAND ${KCC_EXECUTABLE} -help)

aux_source_directory(${CMAKE_SOURCE_DIR}/test/usual USUAL_FILES)

foreach(USUAL_FILE ${USUAL_FILES})
  get_filename_component(USUAL_FILE_NAME ${USUAL_FILE} NAME_WE)
  if(USUAL_FILE_NAME MATCHES "testmain")
    continue()
  endif()

  add_test(
    NAME "COMPILE--${USUAL_FILE_NAME}"
    COMMAND
      ${KCC_EXECUTABLE} ${USUAL_FILE} ${CMAKE_SOURCE_DIR}/test/usual/testmain.c
      -O0 -g -std=gnu17 -o ${TEST_BINARY_DIR}/${USUAL_FILE_NAME})
  add_test(NAME "RUN--${USUAL_FILE_NAME}"
           COMMAND ${TEST_BINARY_DIR}/${USUAL_FILE_NAME})

  add_test(
    NAME "COMPILE--${USUAL_FILE_NAME}--OPT"
    COMMAND
      ${KCC_EXECUTABLE} ${USUAL_FILE} ${CMAKE_SOURCE_DIR}/test/usual/testmain.c
      -O3 -std=gnu17 -o ${TEST_BINARY_DIR}/${USUAL_FILE_NAME}_opt)
  add_test(NAME "RUN--${USUAL_FILE_NAME}--OPT"
           COMMAND ${TEST_BINARY_DIR}/${USUAL_FILE_NAME}_opt)
endforeach()

add_test(NAME "COMPILE--ZCC"
         COMMAND ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/zcc/*.c -O0 -g
                 -std=gnu17 -o ${TEST_BINARY_DIR}/zcc)

add_test(NAME "COMPILE--ZCC--OPT"
         COMMAND ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/zcc/*.c -O3
                 -std=gnu17 -o ${TEST_BINARY_DIR}/zcc_opt)

add_test(
  NAME "COMPILE--8CC"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/8cc/*.c -O0 -g -std=gnu17
    -DBUILD_DIR=\"${CMAKE_SOURCE_DIR}/test/8cc\" -o ${TEST_BINARY_DIR}/8cc)
add_test(NAME check_8cc_executable COMMAND ${TEST_BINARY_DIR}/8cc -h)

add_test(
  NAME "COMPILE--8CC--OPT"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/8cc/*.c -O3 -std=gnu17
    -DBUILD_DIR=\"${CMAKE_SOURCE_DIR}/test/8cc\" -o ${TEST_BINARY_DIR}/8cc_opt)
add_test(NAME check_8cc_opt_executable COMMAND ${TEST_BINARY_DIR}/8cc_opt -h)

set(TEST_OBJ_DIR ${CMAKE_BINARY_DIR}/objs)
file(MAKE_DIRECTORY ${TEST_OBJ_DIR})
aux_source_directory(${CMAKE_SOURCE_DIR}/test/8cc 8CC_FILES)

foreach(8CC_FILE ${8CC_FILES})
  get_filename_component(8CC_FILE_NAME ${8CC_FILE} NAME_WE)

  add_test(
    NAME "COMPILE--${8CC_FILE_NAME}_8cc"
    COMMAND
      ${TEST_BINARY_DIR}/8cc ${8CC_FILE}
      -DBUILD_DIR=\"${CMAKE_SOURCE_DIR}/test/8cc\" -c -o
      ${TEST_OBJ_DIR}/${8CC_FILE_NAME}.o)

  add_test(
    NAME "COMPILE--${8CC_FILE_NAME}_8cc_opt"
    COMMAND
      ${TEST_BINARY_DIR}/8cc_opt ${8CC_FILE}
      -DBUILD_DIR=\"${CMAKE_SOURCE_DIR}/test/8cc\" -c -o
      ${TEST_OBJ_DIR}/${8CC_FILE_NAME}.o)
endforeach()

add_test(
  NAME "COMPILE--LUA"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/lua/*.c -O0 -g -std=gnu17
    -DLUA_USER_H=\"ltests.h\" -DLUA_USE_LINUX -DLUA_COMPAT_5_2 -ldl -lreadline
    -lm -o ${TEST_BINARY_DIR}/lua)
add_test(NAME check_lua_executable COMMAND ${TEST_BINARY_DIR}/lua -v)

add_test(
  NAME "COMPILE--LUA--OPT"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/lua/*.c -O3 -std=gnu17
    -DLUA_USER_H=\"ltests.h\" -DLUA_USE_LINUX -DLUA_COMPAT_5_2 -ldl -lreadline
    -lm -o ${TEST_BINARY_DIR}/lua_opt)
add_test(NAME check_lua_opt_executable COMMAND ${TEST_BINARY_DIR}/lua_opt -v)

add_test(
  NAME lua_test
  COMMAND ${TEST_BINARY_DIR}/lua ${CMAKE_SOURCE_DIR}/test/lua/testes/all.lua
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test/lua/testes)

add_test(
  NAME lua_test_opt
  COMMAND ${TEST_BINARY_DIR}/lua_opt ${CMAKE_SOURCE_DIR}/test/lua/testes/all.lua
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test/lua/testes)

add_test(
  NAME "COMPILE--SQLITE"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/sqlite/shell.c
    ${CMAKE_SOURCE_DIR}/test/sqlite/sqlite3.c -o ${TEST_BINARY_DIR}/sqlite -O0
    -g -std=gnu17 -lpthread -ldl -lm -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_DQS=0 -DSQLITE_ENABLE_DBSTAT_VTAB -DSQLITE_ENABLE_FTS5
    -DSQLITE_ENABLE_GEOPOLY -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RBU
    -DSQLITE_ENABLE_RTREE -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
    -DSQLITE_MAX_EXPR_DEPTH=0 -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED
    -DSQLITE_USE_ALLOCA -DSQLITE_ENABLE_MEMSYS5)
add_test(NAME check_sqlite_executable COMMAND ${TEST_BINARY_DIR}/sqlite
                                              -version)

add_test(
  NAME "COMPILE--SQLITE--OPT"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/sqlite/shell.c
    ${CMAKE_SOURCE_DIR}/test/sqlite/sqlite3.c -o ${TEST_BINARY_DIR}/sqlite_opt
    -O3 -std=gnu17 -lpthread -ldl -lm -DNDEBUG -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_DQS=0 -DSQLITE_ENABLE_DBSTAT_VTAB -DSQLITE_ENABLE_FTS5
    -DSQLITE_ENABLE_GEOPOLY -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RBU
    -DSQLITE_ENABLE_RTREE -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
    -DSQLITE_MAX_EXPR_DEPTH=0 -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED
    -DSQLITE_USE_ALLOCA -DSQLITE_ENABLE_MEMSYS5)
add_test(NAME check_sqlite_opt_executable COMMAND ${TEST_BINARY_DIR}/sqlite_opt
                                                  -version)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_custom_target(test_all COMMAND ctest -j1 -C Debug -T test
                                     --output-on-failure)
else()
  add_custom_target(test_all COMMAND ctest -j1 -C Release -T test
                                     --output-on-failure)
endif()
