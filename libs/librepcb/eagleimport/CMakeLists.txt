# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(librepcb_eagleimport STATIC
    # Sources
    converterdb.cpp
    deviceconverter.cpp
    devicesetconverter.cpp
    packageconverter.cpp
    symbolconverter.cpp

    # Headers
    converterdb.h
    deviceconverter.h
    devicesetconverter.h
    packageconverter.h
    symbolconverter.h
)
target_include_directories(librepcb_eagleimport PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(librepcb_eagleimport
    # LibrePCB
    LibrePCB::Library
    LibrePCB::Common

    # Third party
    ParsEagle::ParsEagle

    # Qt
    Qt5::Core
)

# In Qt 5.15, a lot of things were marked as deprecated, without providing
# alternatives available in previous Qt versions. It would require a lot of
# preprocessor conditionals to avoid these deprecation warnings, so let's just
# disable them for now. We are using CI anyway to ensure LibrePCB compiles with
# the targeted Qt versions.
if("${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}" VERSION_GREATER 5.14)
    target_compile_options(librepcb_eagleimport PRIVATE -Wno-deprecated-declarations)
endif()

# Alias to namespaced variant
add_library(LibrePCB::EagleImport ALIAS librepcb_eagleimport)
