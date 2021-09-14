# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(librepcb_workspace STATIC
    # Sources
    favoriteprojectsmodel.cpp
    fileiconprovider.cpp
    library/cat/categorytreeitem.cpp
    library/cat/categorytreemodel.cpp
    library/workspacelibrarydb.cpp
    library/workspacelibraryscanner.cpp
    projecttreemodel.cpp
    recentprojectsmodel.cpp
    settings/workspacesettings.cpp
    settings/workspacesettingsdialog.cpp
    settings/workspacesettingsitem.cpp
    workspace.cpp

    # Headers
    favoriteprojectsmodel.h
    fileiconprovider.h
    library/cat/categorytreeitem.h
    library/cat/categorytreemodel.h
    library/workspacelibrarydb.h
    library/workspacelibraryscanner.h
    projecttreemodel.h
    recentprojectsmodel.h
    settings/workspacesettings.h
    settings/workspacesettingsdialog.h
    settings/workspacesettingsitem.h
    settings/workspacesettingsitem_genericvalue.h
    settings/workspacesettingsitem_genericvaluelist.h
    workspace.h

    # Forms
    settings/workspacesettingsdialog.ui
    workspacechooserdialog.ui
)
target_include_directories(librepcb_workspace PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(librepcb_workspace PRIVATE common)
target_link_libraries(librepcb_workspace PUBLIC
    # LibrePCB
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
