set(QUAZIP_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/quazip")
if(EXISTS "${QUAZIP_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_QUAZIP
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored QuaZip")

  # Use same Qt version as for LibrePCB
  set(QUAZIP_QT_MAJOR_VERSION ${QT_MAJOR_VERSION})

  # We don't need bzip2 (do we?)
  set(QUAZIP_BZIP2
      OFF
      CACHE BOOL "" FORCE
  )

  # We don't need to support installation when using the lib as a submodule
  set(QUAZIP_INSTALL
      OFF
      CACHE BOOL "" FORCE
  )

  # Include local submodule
  add_subdirectory(
    "${QUAZIP_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/quazip"
    EXCLUDE_FROM_ALL
  )

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system
#
# NOTE: Due to packaging issues with QuaZip 0.x, we only support QuaZip 1.x
#       when unbundling. Using QuaZip 0.9 should work as well, but then you'll
#       have to patch this find script as well as potentially the include paths
#       (quazip -> quazip5) yourself. Or just use the bundled version. See also:
#       https://github.com/LibrePCB/LibrePCB/pull/798#issuecomment-720167363

find_package(QuaZip-${QT})
if(QuaZip-${QT}_FOUND)
  message(STATUS "Using system QuaZip 1.x")

  # Stop here, we're done
  return()
endif()

message(FATAL_ERROR "Did not find QuaZip 1.x system library")
