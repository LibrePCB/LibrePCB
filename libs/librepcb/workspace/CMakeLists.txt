# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(
  librepcb_workspace STATIC
  favoriteprojectsmodel.cpp
  favoriteprojectsmodel.h
  fileiconprovider.cpp
  fileiconprovider.h
  library/cat/categorytreeitem.cpp
  library/cat/categorytreeitem.h
  library/cat/categorytreemodel.cpp
  library/cat/categorytreemodel.h
  library/workspacelibrarydb.cpp
  library/workspacelibrarydb.h
  library/workspacelibraryscanner.cpp
  library/workspacelibraryscanner.h
  projecttreemodel.cpp
  projecttreemodel.h
  recentprojectsmodel.cpp
  recentprojectsmodel.h
  settings/workspacesettings.cpp
  settings/workspacesettings.h
  settings/workspacesettingsdialog.cpp
  settings/workspacesettingsdialog.h
  settings/workspacesettingsdialog.ui
  settings/workspacesettingsitem.cpp
  settings/workspacesettingsitem.h
  settings/workspacesettingsitem_genericvalue.h
  settings/workspacesettingsitem_genericvaluelist.h
  workspace.cpp
  workspace.h
  workspacechooserdialog.ui
)
target_include_directories(
  librepcb_workspace
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_workspace
  PRIVATE common
)
target_link_libraries(
  librepcb_workspace
  PUBLIC # LibrePCB
         LibrePCB::Project
         # Third party
         TypeSafe::TypeSafe
         Optional::Optional
         # Qt
         Qt5::Sql
         Qt5::Widgets
)

# Alias to namespaced variant
add_library(LibrePCB::Workspace ALIAS librepcb_workspace)
