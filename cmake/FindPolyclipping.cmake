set(POLYCLIPPING_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/polyclipping")
if(EXISTS "${POLYCLIPPING_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_POLYCLIPPING
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored Polyclipping")

  # Add library
  add_subdirectory(
    "${POLYCLIPPING_SUBMODULE_BASEPATH}"
    "${CMAKE_BINARY_DIR}/libs/polyclipping" EXCLUDE_FROM_ALL
  )

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system via pkg-config
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(Polyclipping GLOBAL IMPORTED_TARGET polyclipping)
endif()
if(Polyclipping_FOUND)
  message(STATUS "Using system Polyclipping (via pkg-config)")
  add_library(Polyclipping::Polyclipping ALIAS PkgConfig::Polyclipping)
  return()
endif()

message(FATAL_ERROR "Did not find Polyclipping system library via pkg-config")

# Here we could search for the library manually, using find_path etc
