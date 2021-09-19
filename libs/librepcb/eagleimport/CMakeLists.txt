# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(
  librepcb_eagleimport STATIC
  converterdb.cpp
  converterdb.h
  deviceconverter.cpp
  deviceconverter.h
  devicesetconverter.cpp
  devicesetconverter.h
  packageconverter.cpp
  packageconverter.h
  symbolconverter.cpp
  symbolconverter.h
)
target_include_directories(
  librepcb_eagleimport
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_eagleimport
  PRIVATE common
)
target_link_libraries(
  librepcb_eagleimport
  PUBLIC # LibrePCB
         LibrePCB::Library
         LibrePCB::Common
         # Third party
         ParsEagle::ParsEagle
         # Qt
         Qt5::Core
)

# Alias to namespaced variant
add_library(LibrePCB::EagleImport ALIAS librepcb_eagleimport)
