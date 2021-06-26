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
target_link_libraries(librepcb_workspace
    # LibrePCB
    LibrePCB::Project

    # Third party
    TypeSafe::TypeSafe
    Optional::Optional

    # Qt
    Qt5::Sql
    Qt5::Widgets
)

# In Qt 5.15, a lot of things were marked as deprecated, without providing
# alternatives available in previous Qt versions. It would require a lot of
# preprocessor conditionals to avoid these deprecation warnings, so let's just
# disable them for now. We are using CI anyway to ensure LibrePCB compiles with
# the targeted Qt versions.
if("${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}" VERSION_GREATER 5.14)
    target_compile_options(librepcb_workspace PRIVATE -Wno-deprecated-declarations)
endif()

# Alias to namespaced variant
add_library(LibrePCB::Workspace ALIAS librepcb_workspace)
