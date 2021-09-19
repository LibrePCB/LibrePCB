set(OPTIONAL_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/optional")
if(EXISTS "${OPTIONAL_SUBMODULE_BASEPATH}")
  message(STATUS "Using vendored Optional")

  # Disable tests
  set(OPTIONAL_ENABLE_TESTS
      OFF
      CACHE BOOL "Build tests of the 'optional' library" FORCE
  )

  # Include local submodule
  add_subdirectory(
    "${OPTIONAL_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/optional"
    EXCLUDE_FROM_ALL
  )

  # Alias static lib to namespaced variant
  add_library(Optional::Optional ALIAS optional)

  # Stop here, we're done
  return()
endif()

message(
  FATAL_ERROR
    "Did not find Optional library submodule, did you clone recursively?"
)
