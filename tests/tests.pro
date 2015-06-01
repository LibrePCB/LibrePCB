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

QT += core
QT -= gui widgets

CONFIG += console
CONFIG -= app_bundle

LIBS += \
    -L$${DESTDIR} \
    -lgmock \
    -leda4ulibrary \    # Note: The order of the libraries is very important for the linker!
    -leda4ucommon       # Another order could end up in "undefined reference" errors!

INCLUDEPATH += \
    ../3rdparty/gmock/gtest/include \
    ../3rdparty/gmock/include \
    ../lib

DEPENDPATH += \
    ../lib/eda4ulibrary \
    ../lib/eda4ucommon

PRE_TARGETDEPS += \
    $${DESTDIR}/libgmock.a \
    $${DESTDIR}/libeda4ulibrary.a \
    $${DESTDIR}/libeda4ucommon.a

SOURCES += main.cpp \
    common/filepathtest.cpp

HEADERS +=
