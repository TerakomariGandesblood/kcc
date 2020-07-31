if(${CLANG_TIDY})
  message(STATUS "Enable clang-tidy")
  set(CMAKE_CXX_CLANG_TIDY clang-tidy -header-filter=.*)
endif()
