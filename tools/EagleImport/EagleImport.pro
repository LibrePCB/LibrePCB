#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = eagle-import

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

# Path to UUID_List.ini
isEmpty(UUID_LIST_FILEPATH):UUID_LIST_FILEPATH = $$absolute_path("UUID_List.ini")
DEFINES += UUID_LIST_FILEPATH=\\\"$${UUID_LIST_FILEPATH}\\\"

# Define the application version here (needed for XML files)
DEFINES += APP_VERSION_MAJOR=0
DEFINES += APP_VERSION_MINOR=1

QT += core widgets opengl webkitwidgets xml printsupport sql

LIBS += \
    -L$${DESTDIR} \
    -leda4ulibrary \    # Note: The order of the libraries is very important for the linker!
    -leda4ucommon       # Another order could end up in "undefined reference" errors!

INCLUDEPATH += \
    ../../lib

DEPENDPATH += \
    ../../lib/eda4ulibrary \
    ../../lib/eda4ucommon

PRE_TARGETDEPS += \
    $${DESTDIR}/libeda4ulibrary.a \
    $${DESTDIR}/libeda4ucommon.a

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    polygonsimplifier.cpp

HEADERS += \
    mainwindow.h \
    polygonsimplifier.h

FORMS += mainwindow.ui
