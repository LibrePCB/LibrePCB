# Export library
add_library(
  clipper2 STATIC
  include/clipper2/clipper.core.h
  include/clipper2/clipper.engine.h
  include/clipper2/clipper.export.h
  include/clipper2/clipper.h
  include/clipper2/clipper.minkowski.h
  include/clipper2/clipper.offset.h
  include/clipper2/clipper.rectclip.h
  include/clipper2/clipper.version.h
  src/clipper.engine.cpp
  src/clipper.offset.cpp
  src/clipper.rectclip.cpp
)
target_include_directories(
  clipper2
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Alias to namespaced variant
add_library(Clipper2::Clipper2 ALIAS clipper2)
