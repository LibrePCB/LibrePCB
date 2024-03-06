set(DXFLIB_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/dxflib")
if(EXISTS "${DXFLIB_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_DXFLIB
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored Dxflib")

  # Include local submodule
  add_subdirectory(
    "${DXFLIB_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/dxflib"
    EXCLUDE_FROM_ALL
  )

  # Disable deprecation warnings since they are not under our control.
  target_compile_options(dxflib PRIVATE -Wno-deprecated-declarations)

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system via pkg-config
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(Dxflib GLOBAL IMPORTED_TARGET dxflib)
endif()
if(Dxflib_FOUND)
  message(STATUS "Using system Dxflib (via pkg-config)")
  add_library(Dxflib::Dxflib ALIAS PkgConfig::Dxflib)
  return()
endif()

message(FATAL_ERROR "Did not find Dxflib system library")
