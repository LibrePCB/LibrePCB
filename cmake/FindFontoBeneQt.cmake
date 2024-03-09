set(FONTOBENE_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/fontobene-qt")
if(EXISTS "${FONTOBENE_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_FONTOBENE_QT
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored FontoBeneQt")

  # Use same Qt version as for LibrePCB
  set(FONTOBENE_QT_MAJOR_VERSION 5)

  # Include local submodule
  add_subdirectory(
    "${FONTOBENE_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/fontobene-qt"
    EXCLUDE_FROM_ALL
  )

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(FONTOBENE fontobene-qt)
endif()
if(FONTOBENE_FOUND)
  message(STATUS "Using system FontoBeneQt (via pkg-config)")

  # Declare target
  add_library(fontobene_qt INTERFACE)
  target_include_directories(fontobene_qt INTERFACE "${FONTOBENE_INCLUDE_DIRS}")
  target_compile_options(fontobene_qt INTERFACE ${FONTOBENE_CFLAGS_OTHER})

  # Alias lib to namespaced variant
  add_library(FontoBene::FontoBeneQt ALIAS fontobene_qt)

  # Stop here, we're done
  return()
endif()

# If that fails, search for the library directly
find_path(
  FONTOBENE_INCLUDE_DIR
  NAMES fontobene-qt/fontobene.h
  HINTS ${CMAKE_INSTALL_INCLUDEDIR}
)
if(FONTOBENE_INCLUDE_DIR)
  message(STATUS "Using system FontoBeneQt (via find_path)")

  add_library(fontobene_qt INTERFACE)
  target_include_directories(fontobene_qt INTERFACE "${FONTOBENE_INCLUDE_DIR}")
  target_compile_options(fontobene_qt INTERFACE ${FONTOBENE_CFLAGS_OTHER})

  # Alias lib to namespaced variant
  add_library(FontoBene::FontoBeneQt ALIAS fontobene_qt)

  # Mark intermediate vars as advanced
  mark_as_advanced(FONTOBENE_INCLUDE_DIR)

  # Stop here, we're done
  return()
endif()

# Fall back to fontobene-qt5 for backwards compatibility and do the same again
message(
  WARNING
    "fontobene-qt not found, looking for the deprecated fontobene-qt5 instead. \
    Please make sure fontobene-qt is installed as support for fontobene-qt5 \
    will be removed in the next release."
)

# Try to find shared library on the system
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(FONTOBENE fontobene-qt5)
endif()
if(FONTOBENE_FOUND)
  message(STATUS "Using system FontoBeneQt5 (via pkg-config)")

  # Declare target
  add_library(fontobene_qt INTERFACE)
  target_include_directories(fontobene_qt INTERFACE "${FONTOBENE_INCLUDE_DIRS}")
  target_compile_options(
    fontobene_qt INTERFACE -DFONTOBENE_QT5=1 ${FONTOBENE_CFLAGS_OTHER}
  )

  # Alias lib to namespaced variant
  add_library(FontoBene::FontoBeneQt ALIAS fontobene_qt)

  # Stop here, we're done
  return()
endif()

# If that fails, search for the library directly
find_path(
  FONTOBENE_INCLUDE_DIR
  NAMES fontobene-qt5/fontobene.h
  HINTS ${CMAKE_INSTALL_INCLUDEDIR}
)
if(FONTOBENE_INCLUDE_DIR)
  message(STATUS "Using system FontoBeneQt5 (via find_path)")

  add_library(fontobene_qt INTERFACE)
  target_include_directories(fontobene_qt INTERFACE "${FONTOBENE_INCLUDE_DIR}")
  target_compile_options(
    fontobene_qt INTERFACE -DFONTOBENE_QT5=1 ${FONTOBENE_CFLAGS_OTHER}
  )

  # Alias lib to namespaced variant
  add_library(FontoBene::FontoBeneQt ALIAS fontobene_qt)

  # Mark intermediate vars as advanced
  mark_as_advanced(FONTOBENE_INCLUDE_DIR)

  # Stop here, we're done
  return()
endif()

message(FATAL_ERROR "Did not find FontoBeneQt system library")
