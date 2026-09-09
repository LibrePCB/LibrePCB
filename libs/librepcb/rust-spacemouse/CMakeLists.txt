# Claude AI assisted in the writing of this file.
# It was reviewed by a human.

# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# On Linux (and other non-Windows, non-macOS systems), the `hidapi` crate's
# hidraw backend links against libudev at build time. Treat this as a soft
# dependency rather than a hard build failure: if libudev isn't available,
# skip building space mouse support and warn the user.  Someone building
# LibrePCB from source on such a system likely knows how to install a
# missing library if they want the feature.
set(LIBREPCB_SPACEMOUSE_AVAILABLE TRUE)
if(NOT WIN32 AND NOT APPLE)
  find_library(UDEV_LIBRARY udev)
  if(NOT UDEV_LIBRARY)
    message(WARNING
      "libudev not found. Space mouse support will not be available in "
      "the build. If space mouse support is desired, install libudev and "
      "repeat the build.")
    set(LIBREPCB_SPACEMOUSE_AVAILABLE FALSE)
  endif()
endif()
set(LIBREPCB_SPACEMOUSE_AVAILABLE ${LIBREPCB_SPACEMOUSE_AVAILABLE} PARENT_SCOPE)

if(LIBREPCB_SPACEMOUSE_AVAILABLE)
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
  if(NOT WIN32 AND NOT APPLE)
    target_link_libraries(
      librepcb_rust_spacemouse
      INTERFACE ${UDEV_LIBRARY}
    )
  endif()

  # Alias to namespaced variant
  add_library(LibrePCB::RustSpaceMouse ALIAS librepcb_rust_spacemouse)
endif()
