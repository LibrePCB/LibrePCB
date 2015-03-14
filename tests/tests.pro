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

INCLUDEPATH += \
    ../src

SOURCES += main.cpp \
    ../src/common/file_io/filepath.cpp \
    ../src/common/debug.cpp \
    ../src/common/exceptions.cpp

HEADERS += \
    ../src/common/file_io/filepath.h \
    ../src/common/debug.h \
    ../src/common/exceptions.h
