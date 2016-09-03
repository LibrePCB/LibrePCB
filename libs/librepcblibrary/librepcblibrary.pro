#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibrary

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../

HEADERS += \
    cat/componentcategory.h \
    cat/packagecategory.h \
    pkg/footprint.h \
    pkg/package.h \
    sym/symbol.h \
    sym/symbolpin.h \
    sym/symbolpinpreviewgraphicsitem.h \
    sym/symbolpreviewgraphicsitem.h \
    librarybaseelement.h \
    libraryelement.h \
    libraryelementattribute.h \
    pkg/footprintpad.h \
    cat/librarycategory.h \
    elements.h \
    dev/device.h \
    cmp/component.h \
    cmp/componentsignal.h \
    cmp/componentsymbolvariant.h \
    cmp/componentsymbolvariantitem.h \
    pkg/packagepad.h \
    pkg/footprintpadtht.h \
    pkg/footprintpadsmt.h \
    cmp/componentpinsignalmapitem.h \
    pkg/footprintpadpreviewgraphicsitem.h \
    pkg/footprintpreviewgraphicsitem.h

SOURCES += \
    cat/componentcategory.cpp \
    cat/packagecategory.cpp \
    pkg/footprint.cpp \
    pkg/package.cpp \
    sym/symbol.cpp \
    sym/symbolpin.cpp \
    sym/symbolpinpreviewgraphicsitem.cpp \
    sym/symbolpreviewgraphicsitem.cpp \
    librarybaseelement.cpp \
    libraryelement.cpp \
    libraryelementattribute.cpp \
    pkg/footprintpad.cpp \
    cat/librarycategory.cpp \
    dev/device.cpp \
    cmp/componentsignal.cpp \
    cmp/componentsymbolvariant.cpp \
    cmp/componentsymbolvariantitem.cpp \
    cmp/component.cpp \
    pkg/packagepad.cpp \
    pkg/footprintpadtht.cpp \
    pkg/footprintpadsmt.cpp \
    cmp/componentpinsignalmapitem.cpp \
    pkg/footprintpadpreviewgraphicsitem.cpp \
    pkg/footprintpreviewgraphicsitem.cpp

