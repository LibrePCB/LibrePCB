set(DT_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/delaunay-triangulation")
if(EXISTS "${DT_SUBMODULE_BASEPATH}")
  message(STATUS "Using vendored DelaunayTriangulation")

  # Add library
  add_library(delaunay_triangulation INTERFACE)
  target_include_directories(
    delaunay_triangulation INTERFACE "${DT_SUBMODULE_BASEPATH}"
  )

  # Alias lib to namespaced variant
  add_library(
    DelaunayTriangulation::DelaunayTriangulation ALIAS delaunay_triangulation
  )

  # Stop here, we're done
  return()
endif()

message(
  FATAL_ERROR
    "Did not find DelaunayTriangulation library submodule, did you clone recursively?"
)
