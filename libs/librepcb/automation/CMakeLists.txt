# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(
  librepcb_automation STATIC
  boardtools.cpp
  boardtools.h
  cmdnetclassrulesedit.cpp
  cmdnetclassrulesedit.h
  designsession.cpp
  designsession.h
  librarytools.cpp
  librarytools.h
  mcpserver.cpp
  mcpserver.h
  mcpstdiotransport.cpp
  mcpstdiotransport.h
  projecttools.cpp
  projecttools.h
  schematictools.cpp
  schematictools.h
  toolhelpers.cpp
  toolhelpers.h
)
target_include_directories(
  librepcb_automation
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_automation
  PRIVATE common
)
target_link_libraries(
  librepcb_automation
  PUBLIC # LibrePCB
         LibrePCB::Core
         LibrePCB::Editor
         # Qt
         ${QT}::Core
)

# Alias to namespaced variant
add_library(LibrePCB::Automation ALIAS librepcb_automation)
