set(FONTOBENE_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/fontobene-qt5")
if(EXISTS "${FONTOBENE_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_FONTOBENE_QT5
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored FontoBeneQt5")

  # Include local submodule
  add_subdirectory(
    "${FONTOBENE_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/fontobene-qt5"
    EXCLUDE_FROM_ALL
  )

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
  pkg_check_modules(FONTOBENE fontobene-qt5)
endif()
if(FONTOBENE_FOUND)
  message(STATUS "Using system FontoBeneQt5 (via pkg-config)")

  # Declare target
  add_library(fontobene_qt5 INTERFACE)
  target_include_directories(
    fontobene_qt5 INTERFACE "${FONTOBENE_INCLUDE_DIRS}"
  )
  target_compile_options(fontobene_qt5 INTERFACE ${FONTOBENE_CFLAGS_OTHER})

  # Alias lib to namespaced variant
  add_library(FontoBene::FontoBeneQt5 ALIAS fontobene_qt5)

  # Stop here, we're done
  return()
endif()

# If that fails, search for the library directly
find_path(
  FONTOBENE_INCLUDE_DIR
  NAMES fontobene-qt5/fontobene.h fontobene-qt/fontobene.h fontobene/fontobene.h
  HINTS ${CMAKE_INSTALL_INCLUDEDIR}
)
if(FONTOBENE_INCLUDE_DIR)
  message(STATUS "Using system FontoBeneQt5 (via find_path)")

  add_library(fontobene_qt5 INTERFACE)
  target_include_directories(fontobene_qt5 INTERFACE "${FONTOBENE_INCLUDE_DIR}")
  target_compile_options(fontobene_qt5 INTERFACE ${FONTOBENE_CFLAGS_OTHER})

  # Alias lib to namespaced variant
  add_library(FontoBene::FontoBeneQt5 ALIAS fontobene_qt5)

  # Mark intermediate vars as advanced
  mark_as_advanced(FONTOBENE_INCLUDE_DIR)

  # Stop here, we're done
  return()
endif()

message(FATAL_ERROR "Did not find FontoBeneQt5 system library")
