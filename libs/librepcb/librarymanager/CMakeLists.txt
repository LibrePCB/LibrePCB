# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Export library
add_library(librepcb_librarymanager STATIC
    # Sources
    addlibrarywidget.cpp
    librarydownload.cpp
    libraryinfowidget.cpp
    librarylistwidgetitem.cpp
    librarymanager.cpp
    repositorylibrarylistwidgetitem.cpp

    # Headers
    addlibrarywidget.h
    librarydownload.h
    libraryinfowidget.h
    librarylistwidgetitem.h
    librarymanager.h
    repositorylibrarylistwidgetitem.h

    # Forms
    addlibrarywidget.ui
    libraryinfowidget.ui
    librarylistwidgetitem.ui
    librarymanager.ui
    repositorylibrarylistwidgetitem.ui
)
target_include_directories(librepcb_librarymanager PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(librepcb_librarymanager PRIVATE common)
target_link_libraries(librepcb_librarymanager PUBLIC
    # LibrePCB
    LibrePCB::LibraryEditor

    # Third party
    TypeSafe::TypeSafe
    Optional::Optional

    # Qt
    Qt5::Network
    Qt5::Widgets
)

# Alias to namespaced variant
add_library(LibrePCB::LibraryManager ALIAS librepcb_librarymanager)
