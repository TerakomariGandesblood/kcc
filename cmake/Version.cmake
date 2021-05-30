# https://github.com/gabime/spdlog/blob/v1.x/cmake/utils.cmake
function(kcc_extract_version)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/version.h" FILE_CONTENTS)

  string(REGEX MATCH "KCC_VER_MAJOR ([0-9]+)" _ ${FILE_CONTENTS})
  if(NOT (CMAKE_MATCH_COUNT EQUAL 1))
    message(FATAL_ERROR "Could not extract major version number from version.h")
  endif()
  set(VER_MAJOR ${CMAKE_MATCH_1})

  string(REGEX MATCH "KCC_VER_MINOR ([0-9]+)" _ ${FILE_CONTENTS})
  if(NOT (CMAKE_MATCH_COUNT EQUAL 1))
    message(FATAL_ERROR "Could not extract minor version number from version.h")
  endif()
  set(VER_MINOR ${CMAKE_MATCH_1})

  string(REGEX MATCH "KCC_VER_PATCH ([0-9]+)" _ ${FILE_CONTENTS})
  if(NOT (CMAKE_MATCH_COUNT EQUAL 1))
    message(FATAL_ERROR "Could not extract patch version number from version.h")
  endif()
  set(VER_PATCH ${CMAKE_MATCH_1})

  set(KCC_VERSION_MAJOR
      ${VER_MAJOR}
      PARENT_SCOPE)
  set(KCC_VERSION_MINOR
      ${VER_MINOR}
      PARENT_SCOPE)
  set(KCC_VERSION_PATCH
      ${VER_PATCH}
      PARENT_SCOPE)
  set(KCC_VERSION
      ${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}
      PARENT_SCOPE)
endfunction()
