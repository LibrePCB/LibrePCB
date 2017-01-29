#-------------------------------------------------
#
# Project created 2014-08-02
#
#-------------------------------------------------

TEMPLATE = app
TARGET = tests

# Set the path for the generated binary
GENERATED_DIR = ../generated

# Use common project definitions
include(../common.pri)

QT += core widgets network printsupport xml opengl sql concurrent

CONFIG += console
CONFIG -= app_bundle

LIBS += \
    -L$${DESTDIR} \
    -lgoogletest \
    -llibrepcbproject \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon \     # Another order could end up in "undefined reference" errors!
    -lquazip -lz

INCLUDEPATH += \
    ../libs/googletest/googletest/include \
    ../libs/googletest/googlemock/include \
    ../libs/quazip \
    ../libs

DEPENDPATH += \
    ../libs/librepcbproject \
    ../libs/librepcblibrary \
    ../libs/librepcbcommon \
    ../libs/quazip \

PRE_TARGETDEPS += \
    $${DESTDIR}/libgoogletest.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libquazip.a

SOURCES += \
    common/applicationtest.cpp \
    common/directorylocktest.cpp \
    common/filedownloadtest.cpp \
    common/filepathtest.cpp \
    common/networkrequesttest.cpp \
    common/pointtest.cpp \
    common/scopeguardtest.cpp \
    common/sqlitedatabasetest.cpp \
    common/systeminfotest.cpp \
    common/uuidtest.cpp \
    common/versiontest.cpp \
    main.cpp \
    project/projecttest.cpp \

HEADERS += \
    common/networkrequestbasesignalreceiver.h \

