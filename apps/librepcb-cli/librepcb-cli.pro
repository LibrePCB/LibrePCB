#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T16:47:16
#
#-------------------------------------------------

TEMPLATE = app
TARGET = librepcb-cli

# Use common project definitions
include(../../common.pri)

QT += core widgets opengl network xml printsupport sql

CONFIG += console

# Files to be installed by "make install"
target.path = $${PREFIX}/bin
INSTALLS += target

# Note: The order of the libraries is very important for the linker!
# Another order could end up in "undefined reference" errors!
LIBS += \
    -L$${DESTDIR} \
    -lhoedown \
    -llibrepcblibrarymanager \
    -llibrepcbprojecteditor \
    -llibrepcblibraryeditor \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \
    -llibrepcbcommon \
    -lsexpresso \
    -lclipper \
    -lmuparser \
    -lquazip -lz

INCLUDEPATH += \
    ../../libs \
    ../../libs/quazip \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/hoedown \
    ../../libs/librepcb/librarymanager \
    ../../libs/librepcb/projecteditor \
    ../../libs/librepcb/libraryeditor \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/quazip \
    ../../libs/sexpresso \
    ../../libs/clipper \
    ../../libs/muparser \

PRE_TARGETDEPS += \
    $${DESTDIR}/libhoedown.a \
    $${DESTDIR}/liblibrepcblibrarymanager.a \
    $${DESTDIR}/liblibrepcbprojecteditor.a \
    $${DESTDIR}/liblibrepcblibraryeditor.a \
    $${DESTDIR}/liblibrepcbworkspace.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libquazip.a \
    $${DESTDIR}/libsexpresso.a \
    $${DESTDIR}/libclipper.a \
    $${DESTDIR}/libmuparser.a \

RESOURCES += \
    ../../img/images.qrc \

SOURCES += \
    commandlineinterface.cpp \
    main.cpp \

HEADERS += \
    commandlineinterface.h \

