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

QT += core widgets xml network

LIBS += \
    -L$${DESTDIR} \
    -llibrepcbeagleimport \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon \     # Another order could end up in "undefined reference" errors!
    -lparseagle

INCLUDEPATH += \
    ../../libs \
    ../../libs/parseagle

DEPENDPATH += \
    ../../libs/librepcb/eagleimport \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/parseagle

PRE_TARGETDEPS += \
    $${DESTDIR}/liblibrepcbeagleimport.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libparseagle.a

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    polygonsimplifier.cpp \

HEADERS += \
    mainwindow.h \
    polygonsimplifier.h \

FORMS += \
    mainwindow.ui \

