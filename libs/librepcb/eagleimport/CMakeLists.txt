# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(
  librepcb_eagleimport STATIC eaglelibraryimport.cpp eaglelibraryimport.h
                              eagletypeconverter.cpp eagletypeconverter.h
)
target_include_directories(
  librepcb_eagleimport
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_eagleimport
  PRIVATE common
          # Third party
          ParsEagle::ParsEagle
)
target_link_libraries(
  librepcb_eagleimport
  PUBLIC # LibrePCB
         LibrePCB::Library
         LibrePCB::Common
         # Qt
         Qt5::Core
)

# Alias to namespaced variant
add_library(LibrePCB::EagleImport ALIAS librepcb_eagleimport)
