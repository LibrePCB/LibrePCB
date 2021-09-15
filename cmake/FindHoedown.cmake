set(HOEDOWN_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/hoedown")
if(EXISTS "${HOEDOWN_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_HOEDOWN
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored Hoedown")

  # Add library
  add_subdirectory(
    "${HOEDOWN_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/hoedown"
    EXCLUDE_FROM_ALL
  )

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system via pkg-config
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(Hoedown GLOBAL IMPORTED_TARGET hoedown)
endif()
if(Hoedown_FOUND)
  message(STATUS "Using system Hoedown (via pkg-config)")
  add_library(Hoedown::Hoedown ALIAS PkgConfig::Hoedown)
  return()
endif()

message(FATAL_ERROR "Did not find Hoedown system library via pkg-config")

# Here we could search for the library manually, using find_path etc
