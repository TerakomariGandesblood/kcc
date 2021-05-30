# https://github.com/gabime/spdlog/blob/v1.x/CMakeLists.txt
cmake_minimum_required(VERSION 3.19)
message(STATUS "CMake version: ${CMAKE_VERSION}")

# ---------------------------------------------------------------------------------------
# Start kcc project
# ---------------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "/usr/lib/llvm-12/lib/cmake/lld")

include(Version)
kcc_extract_version()

project(
  kcc
  VERSION ${KCC_VERSION}
  LANGUAGES CXX)

message(STATUS "${PROJECT_NAME} version: ${KCC_VERSION}")

# ---------------------------------------------------------------------------------------
# Set default build to release
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose Debug, Release, MinSizeRel or RelWithDebInfo" FORCE)
endif()
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# ---------------------------------------------------------------------------------------
# Set variables
# ---------------------------------------------------------------------------------------
set(LIBRARY lib${PROJECT_NAME})
set(EXECUTABLE ${PROJECT_NAME})

set(KCC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(KCC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE LIBRARY_SRC CONFIGURE_DEPENDS "${KCC_SOURCE_DIR}/src/*.cpp")

# ---------------------------------------------------------------------------------------
# Include CMake module
# ---------------------------------------------------------------------------------------
include(CTest)
include(GNUInstallDirs)

include(Options)
include(Check)
include(ClangTidy)
include(Format)
include(CompilerOptions)

# ---------------------------------------------------------------------------------------
# Find package
# ---------------------------------------------------------------------------------------
find_package(LLVM 12 REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(LLD REQUIRED CONFIG)
find_package(fmt REQUIRED)
find_package(Boost 1.75.0 REQUIRED json)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------------------
# Build static library
# ---------------------------------------------------------------------------------------
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

add_library(${LIBRARY} STATIC ${LIBRARY_SRC})
target_include_directories(${LIBRARY} PUBLIC "${KCC_SOURCE_DIR}/include")
target_link_libraries(
  ${LIBRARY}
  PRIVATE ${CMAKE_THREAD_LIBS_INIT}
          ${ICU_LIBRARIES}
          ${Boost_LIBRARIES}
          clang-cpp
          LLVM
          lldELF
          lldCommon
          fmt::fmt)

set_target_properties(
  ${LIBRARY}
  PROPERTIES DEBUG_POSTFIX d
             OUTPUT_NAME ${PROJECT_NAME}
             POSITION_INDEPENDENT_CODE ON
             INTERPROCEDURAL_OPTIMIZATION FALSE)

# ---------------------------------------------------------------------------------------
# Build executable
# ---------------------------------------------------------------------------------------
add_subdirectory(tool)

# ---------------------------------------------------------------------------------------
# Build test
# ---------------------------------------------------------------------------------------
if(BUILD_TESTING AND KCC_BUILD_TEST)
  message(STATUS "Build tests")
  add_subdirectory(test)
endif()

# ---------------------------------------------------------------------------------------
# Install target
# ---------------------------------------------------------------------------------------
include(Install)

# https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake
configure_file("${KCC_SOURCE_DIR}/cmake/CMakeUninstall.cmake.in"
               "${KCC_BINARY_DIR}/CMakeUninstall.cmake" @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P
                                    ${KCC_BINARY_DIR}/CMakeUninstall.cmake)
