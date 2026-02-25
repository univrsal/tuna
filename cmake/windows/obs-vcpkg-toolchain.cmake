# Chainload the vcpkg CMake toolchain, then restore find root path modes so that
# OBS-bundled dependencies (such as CURL) can still be discovered from CMAKE_PREFIX_PATH.

if(DEFINED ENV{VCPKG_INSTALLATION_ROOT}
   AND EXISTS "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake")
  include("$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake")

  # vcpkg sets CMAKE_FIND_ROOT_PATH_MODE_* to ONLY when the target triplet does not
  # match the host triplet (it treats the build as cross-compilation).  Reset these
  # to BOTH so that packages provided by OBS pre-built dependencies (e.g. CURL) are
  # still found via CMAKE_PREFIX_PATH alongside vcpkg-managed packages.
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()
