# Claude AI assisted in the writing of this file.
# It was reviewed by a human.

# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Determine Cargo features
set(CARGO_FEATURES)
list(APPEND CARGO_FEATURES ffi)
if(BUILD_DISALLOW_WARNINGS)
  list(APPEND CARGO_FEATURES fail-on-warnings)
endif()

# Import Rust crate
corrosion_import_crate(MANIFEST_PATH Cargo.toml FEATURES ${CARGO_FEATURES})

# Export library
target_link_libraries(
  librepcb_rust_spacemouse
  INTERFACE common
)

# Alias to namespaced variant
add_library(LibrePCB::RustSpaceMouse ALIAS librepcb_rust_spacemouse)
