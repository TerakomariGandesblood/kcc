if(KCC_INSTALL)
  message(STATUS "Generate the install target")

  include(GNUInstallDirs)

  # ---------------------------------------------------------------------------------------
  # Install executable
  # ---------------------------------------------------------------------------------------
  if(KCC_BUILD_EXECUTABLE OR KCC_BUILD_ALL)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};$\{ORIGIN\}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
         "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
    if(${isSystemDir} STREQUAL "-1")
      set(CMAKE_INSTALL_RPATH
          "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};$\{ORIGIN\}")
    endif()

    install(TARGETS ${EXECUTABLE} DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  # ---------------------------------------------------------------------------------------
  # Project information
  # ---------------------------------------------------------------------------------------
  set(KCC_VENDOR "kaiser")
  set(KCC_CONTACT "kaiser <KaiserLancelot123@gmail.com>")
  set(KCC_PROJECT_URL "https://github.com/KaiserLancelot/kcc")
  set(KCC_DESCRIPTION_SUMMARY "A small C11 compiler")

  # ---------------------------------------------------------------------------------------
  # Support creation of installable packages
  # ---------------------------------------------------------------------------------------
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
  set(CPACK_INSTALL_CMAKE_PROJECTS ${CMAKE_CURRENT_BINARY_DIR} ${LIBRARY} ALL .)

  set(CPACK_PROJECT_URL ${KCC_PROJECT_URL})
  set(CPACK_PACKAGE_VENDOR ${KCC_VENDOR})
  set(CPACK_PACKAGE_CONTACT ${KCC_CONTACT})
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${KCC_DESCRIPTION_SUMMARY})
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  set(CPACK_PACKAGE_VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
  if(PROJECT_VERSION_TWEAK)
    set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION}.${PROJECT_VERSION_TWEAK})
  endif()
  set(CPACK_PACKAGE_RELOCATABLE
      ON
      CACHE BOOL "Build relocatable package")

  set(CPACK_GENERATOR
      "TGZ;DEB"
      CACHE STRING "Semicolon separated list of generators")

  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "")
  include(InstallRequiredSystemLibraries)

  include(CPack)
endif()
