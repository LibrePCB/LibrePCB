# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Determine Cargo features
set(CARGO_FEATURES)
if(BUILD_DISALLOW_WARNINGS)
  list(APPEND CARGO_FEATURES fail-on-warnings)
endif()

# Import Rust crate
corrosion_import_crate(MANIFEST_PATH Cargo.toml FEATURES ${CARGO_FEATURES})

# Export library
target_link_libraries(
  librepcb_rust_core
  INTERFACE common
)
target_link_libraries(
  librepcb_rust_core
  INTERFACE ${QT}::Core
)

# Add C++ sources
target_sources(librepcb_rust_core PUBLIC ffi.cpp ffi.h)

# Alias to namespaced variant
add_library(LibrePCB::RustCore ALIAS librepcb_rust_core)
