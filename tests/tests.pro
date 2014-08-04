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

PRE_TARGETDEPS += $${DESTDIR}/libgtest.a

INCLUDEPATH += \
    ../3rdparty/gtest/include \
    ../src

LIBS += \
    -L$${DESTDIR} \
    -lgtest

SOURCES += main.cpp \
    ../src/common/debug.cpp \
    ../src/common/exceptions.cpp

HEADERS += \
    ../src/common/debug.h \
    ../src/common/exceptions.h
