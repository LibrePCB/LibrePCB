# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(
  librepcb_kicadimport STATIC
  kicadlibraryconverter.cpp
  kicadlibraryconverter.h
  kicadlibraryimport.cpp
  kicadlibraryimport.h
  kicadtypeconverter.cpp
  kicadtypeconverter.h
  kicadtypes.cpp
  kicadtypes.h
)
target_include_directories(
  librepcb_kicadimport
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_kicadimport
  PUBLIC # LibrePCB
         LibrePCB::Core
         # Qt
         ${QT}::Core
)

# Alias to namespaced variant
add_library(LibrePCB::KiCadImport ALIAS librepcb_kicadimport)
