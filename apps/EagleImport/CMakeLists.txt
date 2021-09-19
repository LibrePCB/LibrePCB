# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Executable
add_executable(
  eagle_import
  main.cpp mainwindow.cpp mainwindow.h mainwindow.ui polygonsimplifier.cpp
  polygonsimplifier.h
)
target_include_directories(
  eagle_import
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  eagle_import
  PRIVATE common
          # LibrePCB
          LibrePCB::Common
          LibrePCB::Library
          LibrePCB::EagleImport
          # Third party
          ParsEagle::ParsEagle
          # Qt
          Qt5::Gui
          Qt5::Widgets
)
set_target_properties(eagle_import PROPERTIES OUTPUT_NAME "eagle-import")
