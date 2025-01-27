set(SLINT_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/slint")
if(EXISTS "${SLINT_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_SLINT
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored Slint")

  # Set compile options
  set(SLINT_FEATURE_BACKEND_QT ON)
  set(SLINT_FEATURE_BACKEND_WINIT OFF)
  set(SLINT_FEATURE_GETTEXT ON)

  # Include local submodule
  set(BUILD_SHARED_LIBS ON)
  add_subdirectory(
    "${SLINT_SUBMODULE_BASEPATH}/api/cpp" "${CMAKE_BINARY_DIR}/libs/slint"
  )
  set(BUILD_SHARED_LIBS OFF)

  # Suppress compiler warning (https://github.com/slint-ui/slint/issues/2681)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Slint INTERFACE -Wno-gnu-anonymous-struct)
  endif()

  # Suppress compiler warning (https://github.com/slint-ui/slint/issues/7358)
  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION
                                            VERSION_LESS 14
  )
    target_compile_options(Slint INTERFACE -Wno-maybe-uninitialized)
  endif()

  # Suppress compiler warning
  # (maybe https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98646)
  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION
                                            VERSION_LESS 12
  )
    target_compile_options(Slint INTERFACE -Wno-nonnull)
  endif()

  # TODO: This needs to be investigated (pops up on macOS CI).
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Slint INTERFACE -Wno-nullability-completeness)
  endif()

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system.
# Note: Currently disabled as in this early stage we have to update often to new
# versions, sometimes not even released yet so we need the bundled submodule.
#find_package(Slint 1.9)
#if(Slint_FOUND)
#  message(STATUS "Using system Slint")
#
#  # Stop here, we're done
#  return()
#endif()

message(FATAL_ERROR "Did not find Slint system library")
