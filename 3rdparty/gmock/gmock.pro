#-------------------------------------------------
#
# Project created 2014-08-02
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = gmock

# Set the path for the generated library
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

CONFIG -= qt app_bundle
CONFIG += staticlib thread

# suppress compiler warnings
QMAKE_CXXFLAGS += -Wno-missing-field-initializers -Wno-unused-variable -Wno-missing-field-initializers
QMAKE_CXXFLAGS_DEBUG += -Wno-missing-field-initializers -Wno-unused-variable -Wno-missing-field-initializers

INCLUDEPATH += \
    gtest \
    gtest/include \
    include

SOURCES += \
    gtest/src/gtest-all.cc \
    src/gmock-all.cc
