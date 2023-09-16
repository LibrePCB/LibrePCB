# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(
  librepcb_gui STATIC
  editorapplication.cpp
  editorapplication.h
  editortab.cpp
  editortab.h
  editorwindow.cpp
  editorwindow.h
  objectlistmodel.cpp
  objectlistmodel.h
  openedproject.cpp
  openedproject.h
)
target_include_directories(
  librepcb_gui
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_gui
  PUBLIC # LibrePCB
         LibrePCB::Core
         LibrePCB::Editor
         # Qt
         ${QT}::Core
         ${QT}::Gui
         ${QT}::Quick
         ${QT}::QuickControls2
)

# Alias to namespaced variant
add_library(LibrePCB::Gui ALIAS librepcb_gui)
