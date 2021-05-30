include(GNUInstallDirs)

# ---------------------------------------------------------------------------------------
# Install executable
# ---------------------------------------------------------------------------------------
# https://stackoverflow.com/questions/30398238/cmake-rpath-not-working-could-not-find-shared-object-file
set_target_properties(
  ${EXECUTABLE} PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
                           INSTALL_RPATH_USE_LINK_PATH TRUE)

install(
  TARGETS ${EXECUTABLE}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# ---------------------------------------------------------------------------------------
# Support creation of installable packages
# ---------------------------------------------------------------------------------------
# https://cmake.org/cmake/help/latest/module/CPack.html
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_INSTALL_CMAKE_PROJECTS ${KCC_BINARY_DIR} ${LIBRARY} ALL .)

# https://cmake.org/cmake/help/latest/cpack_gen/deb.html
set(CPACK_PACKAGE_CONTACT "kaiser <KaiserLancelot123@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A small C11 compiler")
set(CPACK_PACKAGE_VERSION
    ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# https://cmake.org/cmake/help/latest/manual/cpack-generators.7.html
set(CPACK_GENERATOR "TGZ;DEB")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# https://cmake.org/cmake/help/latest/module/InstallRequiredSystemLibraries.html
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
    "/usr/lib/llvm-12/lib/libclang-cpp.so.12;/usr/lib/x86_64-linux-gnu/libLLVM-12.so.1"
)
include(InstallRequiredSystemLibraries)

include(CPack)
