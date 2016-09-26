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

QT += core widgets network printsupport xml opengl

CONFIG += console
CONFIG -= app_bundle

LIBS += \
    -L$${DESTDIR} \
    -lgmock \
    -llibrepcbproject \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon \     # Another order could end up in "undefined reference" errors!
    -lquazip -lz

INCLUDEPATH += \
    ../3rdparty/gmock/gtest/include \
    ../3rdparty/gmock/include \
    ../3rdparty/quazip \
    ../libs

DEPENDPATH += \
    ../libs/librepcbproject \
    ../libs/librepcblibrary \
    ../libs/librepcbcommon \
    ../3rdparty/quazip \

PRE_TARGETDEPS += \
    $${DESTDIR}/libgmock.a \
    $${DESTDIR}/liblibrepcbproject.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a \
    $${DESTDIR}/libquazip.a

SOURCES += main.cpp \
    common/filepathtest.cpp \
    common/pointtest.cpp \
    common/scopeguardtest.cpp \
    common/applicationtest.cpp \
    common/versiontest.cpp \
    common/systeminfotest.cpp \
    common/directorylocktest.cpp \
    common/uuidtest.cpp \
    common/filedownloadtest.cpp \
    common/networkrequesttest.cpp \
    project/projecttest.cpp

HEADERS += \
    common/networkrequestbasesignalreceiver.h
