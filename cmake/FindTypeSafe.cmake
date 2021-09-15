set(TYPE_SAFE_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/type_safe")
if(EXISTS "${TYPE_SAFE_SUBMODULE_BASEPATH}")
  message(STATUS "Using vendored TypeSafe")

  # Include local submodule
  add_subdirectory(
    "${TYPE_SAFE_SUBMODULE_BASEPATH}" "${CMAKE_BINARY_DIR}/libs/type_safe"
    EXCLUDE_FROM_ALL
  )

  # Alias static lib to namespaced variant
  add_library(TypeSafe::TypeSafe ALIAS type_safe)

  # Stop here, we're done
  return()
endif()

message(
  FATAL_ERROR
    "Did not find TypeSafe library submodule, did you clone recursively?"
)
