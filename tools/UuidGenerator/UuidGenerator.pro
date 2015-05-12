#-------------------------------------------------
#
# Project created by QtCreator 2013-08-22T23:02:35
#
#-------------------------------------------------

TEMPLATE = app
TARGET = uuid-generator

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

QT += core widgets

SOURCES += main.cpp \
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
