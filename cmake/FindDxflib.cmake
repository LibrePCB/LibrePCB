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

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system
find_path(Dxflib_INCLUDE_DIR NAMES dl_dxf.h PATH_SUFFIXES dxflib)
find_library(Dxflib_LIBRARY dxflib)
if(Dxflib_INCLUDE_DIR AND Dxflib_LIBRARY)
  add_library(Dxflib SHARED IMPORTED GLOBAL)
  add_library(Dxflib::Dxflib ALIAS Dxflib)
  set_property(TARGET Dxflib PROPERTY IMPORTED_LOCATION "${Dxflib_LIBRARY}")
  target_include_directories(Dxflib INTERFACE "${Dxflib_INCLUDE_DIR}")
  set(Dxflib_FOUND 1)
  mark_as_advanced(Dxflib_INCLUDE_DIR Dxflib_LIBRARY)
  message(STATUS "Using system Dxflib")
  return()
endif()

set(Dxflib_FOUND 0)
message(FATAL_ERROR "Did not find Dxflib system library")
