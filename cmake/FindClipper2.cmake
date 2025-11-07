set(CLIPPER2_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/clipper2")
if(EXISTS "${CLIPPER2_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_CLIPPER2
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored Clipper2")

  # Add library
  add_subdirectory(
    "${CLIPPER2_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/clipper2"
    EXCLUDE_FROM_ALL
  )

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system via pkg-config
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(Clipper2 GLOBAL IMPORTED_TARGET clipper2)
endif()
if(Clipper2_FOUND)
  message(STATUS "Using system Clipper2 (via pkg-config)")
  add_library(Clipper2::Clipper2 ALIAS PkgConfig::Clipper2)
  return()
endif()

message(FATAL_ERROR "Did not find Clipper2 system library via pkg-config")

# Here we could search for the library manually, using find_path etc
