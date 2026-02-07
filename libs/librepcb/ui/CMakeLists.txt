# Intermediate target for appwindow.slint, to avoid the Qt MOC in the editor
# library being run while not all Slint files are being generated yet. See
# https://github.com/LibrePCB/LibrePCB/issues/1680

# Disable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(librepcb_ui STATIC)
slint_target_sources(
  librepcb_ui appwindow.slint
  NAMESPACE ui
  # Generate multiple cpp files to parallelize compilation, but not too many
  # because of excessive RAM usage when compiling those (huge) files. A value
  # of 12 already caused a compiler segfault on the macOS CI...
  COMPILATION_UNITS 8
)
target_link_libraries(
  librepcb_ui
  PRIVATE common
)
target_link_libraries(
  librepcb_ui
  PUBLIC Slint::Slint
)

# Alias to namespaced variant
add_library(LibrePCB::Ui ALIAS librepcb_ui)
