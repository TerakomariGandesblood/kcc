set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES
                                            "RelWithDebInfo"))
    append("--coverage" CMAKE_CXX_FLAGS)
  endif()
else()
  add_link_options(-fuse-ld=lld)

  if((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES
                                            "RelWithDebInfo"))
    append("-fstandalone-debug" CMAKE_CXX_FLAGS)
    append("-fprofile-instr-generate -fcoverage-mapping" CMAKE_CXX_FLAGS)
  endif()
endif()

if((CMAKE_BUILD_TYPE MATCHES "Release") OR (CMAKE_BUILD_TYPE MATCHES
                                            "RelWithDebInfo"))
  append("-flto" CMAKE_CXX_FLAGS)
endif()
