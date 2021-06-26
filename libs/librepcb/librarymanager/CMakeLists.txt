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
target_link_libraries(librepcb_librarymanager
    # LibrePCB
    LibrePCB::LibraryEditor

    # Third party
    TypeSafe::TypeSafe
    Optional::Optional

    # Qt
    Qt5::Network
    Qt5::Widgets
)

# In Qt 5.15, a lot of things were marked as deprecated, without providing
# alternatives available in previous Qt versions. It would require a lot of
# preprocessor conditionals to avoid these deprecation warnings, so let's just
# disable them for now. We are using CI anyway to ensure LibrePCB compiles with
# the targeted Qt versions.
if("${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}" VERSION_GREATER 5.14)
    target_compile_options(librepcb_librarymanager PRIVATE -Wno-deprecated-declarations)
endif()

# Alias to namespaced variant
add_library(LibrePCB::LibraryManager ALIAS librepcb_librarymanager)
