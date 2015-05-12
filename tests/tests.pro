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

PRE_TARGETDEPS += $${DESTDIR}/libgmock.a

INCLUDEPATH += \
    ../3rdparty/gmock/gtest/include \
    ../3rdparty/gmock/include \
    ../src

LIBS += \
    -L$${DESTDIR} \
    -lgmock

SOURCES += main.cpp \
    ../src/common/debug.cpp \
    ../src/common/exceptions.cpp \
    common/filepathtest.cpp \
    ../src/common/filepath.cpp

HEADERS += \
    ../src/common/debug.h \
    ../src/common/exceptions.h \
    ../src/common/filepath.h
