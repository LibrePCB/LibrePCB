#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibrary

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../

HEADERS += \
    cat/componentcategory.h \
    cat/librarycategory.h \
    cat/packagecategory.h \
    cmp/component.h \
    cmp/componentpinsignalmapitem.h \
    cmp/componentsignal.h \
    cmp/componentsymbolvariant.h \
    cmp/componentsymbolvariantitem.h \
    dev/device.h \
    elements.h \
    library.h \
    librarybaseelement.h \
    libraryelement.h \
    pkg/footprint.h \
    pkg/footprintpad.h \
    pkg/footprintpadpreviewgraphicsitem.h \
    pkg/footprintpadsmt.h \
    pkg/footprintpadtht.h \
    pkg/footprintpreviewgraphicsitem.h \
    pkg/package.h \
    pkg/packagepad.h \
    sym/symbol.h \
    sym/symbolpin.h \
    sym/symbolpinpreviewgraphicsitem.h \
    sym/symbolpreviewgraphicsitem.h \

SOURCES += \
    cat/componentcategory.cpp \
    cat/librarycategory.cpp \
    cat/packagecategory.cpp \
    cmp/component.cpp \
    cmp/componentpinsignalmapitem.cpp \
    cmp/componentsignal.cpp \
    cmp/componentsymbolvariant.cpp \
    cmp/componentsymbolvariantitem.cpp \
    dev/device.cpp \
    library.cpp \
    librarybaseelement.cpp \
    libraryelement.cpp \
    pkg/footprint.cpp \
    pkg/footprintpad.cpp \
    pkg/footprintpadpreviewgraphicsitem.cpp \
    pkg/footprintpadsmt.cpp \
    pkg/footprintpadtht.cpp \
    pkg/footprintpreviewgraphicsitem.cpp \
    pkg/package.cpp \
    pkg/packagepad.cpp \
    sym/symbol.cpp \
    sym/symbolpin.cpp \
    sym/symbolpinpreviewgraphicsitem.cpp \
    sym/symbolpreviewgraphicsitem.cpp \

