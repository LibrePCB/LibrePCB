set(DT_SUBMODULE_BASEPATH "${PROJECT_SOURCE_DIR}/libs/delaunay-triangulation")
if(EXISTS "${DT_SUBMODULE_BASEPATH}")
  # Skip if the target already exists, because find_package() was
  # already processed earlier in this configure run. Some IDEs such as Qt
  # Creator inject a CMAKE_PROJECT_INCLUDE_BEFORE hook that gets
  # re-included for every project() call in the tree, including the
  # nested one in libs/corrosion, which was causing this to run twice
  # and fail (see #1812).
  if(NOT TARGET delaunay_triangulation)
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
  endif()

  # Stop here, we're done
  return()
endif()

message(
  FATAL_ERROR
    "Did not find DelaunayTriangulation library submodule, did you clone recursively?"
)
