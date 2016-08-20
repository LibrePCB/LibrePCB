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

QT += core widgets

CONFIG += console
CONFIG -= app_bundle

LIBS += \
    -L$${DESTDIR} \
    -lgmock \
    -llibrepcblibrary \    # Note: The order of the libraries is very important for the linker!
    -llibrepcbcommon       # Another order could end up in "undefined reference" errors!

INCLUDEPATH += \
    ../3rdparty/gmock/gtest/include \
    ../3rdparty/gmock/include \
    ../libs

DEPENDPATH += \
    ../libs/librepcblibrary \
    ../libs/librepcbcommon

PRE_TARGETDEPS += \
    $${DESTDIR}/libgmock.a \
    $${DESTDIR}/liblibrepcblibrary.a \
    $${DESTDIR}/liblibrepcbcommon.a

SOURCES += main.cpp \
    common/filepathtest.cpp \
    common/pointtest.cpp \
    common/scopeguardtest.cpp \
    common/applicationtest.cpp

HEADERS +=
