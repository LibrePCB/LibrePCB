#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = project-library-updater

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

# Define the application version here (needed for XML files)
DEFINES += APP_VERSION_MAJOR=0
DEFINES += APP_VERSION_MINOR=1

QT += core widgets opengl webkitwidgets xml printsupport sql

LIBS += \
    -L$${DESTDIR} \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon       # Another order could end up in "undefined reference" errors!

INCLUDEPATH += \
    ../../libs

DEPENDPATH += \
    ../../libs/librepcblibrary \
    ../../libs/librepcbcommon

PRE_TARGETDEPS += \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += mainwindow.ui
