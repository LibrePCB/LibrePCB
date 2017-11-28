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

QT += core widgets network xml sql printsupport opengl

LIBS += \
    -L$${DESTDIR} \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon \     # Another order could end up in "undefined reference" errors!
    -lsexpresso \
    -lclipper \

INCLUDEPATH += \
    ../../libs

DEPENDPATH += \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/sexpresso \
    ../../libs/clipper \

PRE_TARGETDEPS += \
    $${DESTDIR}/liblibrepcbworkspace.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libsexpresso.a \
    $${DESTDIR}/libclipper.a \

SOURCES += \
    main.cpp \
    mainwindow.cpp \

HEADERS += \
    mainwindow.h \

FORMS += \
    mainwindow.ui \

