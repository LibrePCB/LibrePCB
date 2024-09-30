set(SLINT_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/slint")
if(EXISTS "${SLINT_SUBMODULE_BASEPATH}"
   AND NOT UNBUNDLE_SLINT
   AND NOT UNBUNDLE_ALL
)
  message(STATUS "Using vendored Slint")

  # Set compile options
  set(SLINT_FEATURE_BACKEND_QT ON)
  set(SLINT_FEATURE_BACKEND_WINIT OFF)

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

  # Stop here, we're done
  return()
endif()

# Otherwise, try to find shared library on the system
find_package(Slint 1.8)
if(Slint_FOUND)
  message(STATUS "Using system Slint")

  # Stop here, we're done
  return()
endif()

message(FATAL_ERROR "Did not find Slint system library")
