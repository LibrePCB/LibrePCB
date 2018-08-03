#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibrary

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../ \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

SOURCES += \
    cat/componentcategory.cpp \
    cat/librarycategory.cpp \
    cat/packagecategory.cpp \
    cmp/cmd/cmdcomponentsignaledit.cpp \
    cmp/cmd/cmdcomponentsymbolvariantedit.cpp \
    cmp/cmpsigpindisplaytype.cpp \
    cmp/component.cpp \
    cmp/componentpinsignalmap.cpp \
    cmp/componentsignal.cpp \
    cmp/componentsymbolvariant.cpp \
    cmp/componentsymbolvariantitem.cpp \
    dev/cmd/cmddeviceedit.cpp \
    dev/cmd/cmddevicepadsignalmapitemedit.cpp \
    dev/device.cpp \
    dev/devicepadsignalmap.cpp \
    library.cpp \
    librarybaseelement.cpp \
    libraryelement.cpp \
    pkg/cmd/cmdfootprintedit.cpp \
    pkg/cmd/cmdfootprintpadedit.cpp \
    pkg/cmd/cmdpackagepadedit.cpp \
    pkg/footprint.cpp \
    pkg/footprintgraphicsitem.cpp \
    pkg/footprintpad.cpp \
    pkg/footprintpadgraphicsitem.cpp \
    pkg/footprintpadpreviewgraphicsitem.cpp \
    pkg/footprintpreviewgraphicsitem.cpp \
    pkg/package.cpp \
    pkg/packagepad.cpp \
    sym/cmd/cmdsymbolpinedit.cpp \
    sym/symbol.cpp \
    sym/symbolgraphicsitem.cpp \
    sym/symbolpin.cpp \
    sym/symbolpingraphicsitem.cpp \
    sym/symbolpinpreviewgraphicsitem.cpp \
    sym/symbolpreviewgraphicsitem.cpp \

HEADERS += \
    cat/componentcategory.h \
    cat/librarycategory.h \
    cat/packagecategory.h \
    cmp/cmd/cmdcomponentsignaledit.h \
    cmp/cmd/cmdcomponentsymbolvariantedit.h \
    cmp/cmpsigpindisplaytype.h \
    cmp/component.h \
    cmp/componentpinsignalmap.h \
    cmp/componentprefix.h \
    cmp/componentsignal.h \
    cmp/componentsymbolvariant.h \
    cmp/componentsymbolvariantitem.h \
    cmp/componentsymbolvariantitemsuffix.h \
    dev/cmd/cmddeviceedit.h \
    dev/cmd/cmddevicepadsignalmapitemedit.h \
    dev/device.h \
    dev/devicepadsignalmap.h \
    elements.h \
    library.h \
    librarybaseelement.h \
    libraryelement.h \
    pkg/cmd/cmdfootprintedit.h \
    pkg/cmd/cmdfootprintpadedit.h \
    pkg/cmd/cmdpackagepadedit.h \
    pkg/footprint.h \
    pkg/footprintgraphicsitem.h \
    pkg/footprintpad.h \
    pkg/footprintpadgraphicsitem.h \
    pkg/footprintpadpreviewgraphicsitem.h \
    pkg/footprintpreviewgraphicsitem.h \
    pkg/package.h \
    pkg/packagepad.h \
    sym/cmd/cmdsymbolpinedit.h \
    sym/symbol.h \
    sym/symbolgraphicsitem.h \
    sym/symbolpin.h \
    sym/symbolpingraphicsitem.h \
    sym/symbolpinpreviewgraphicsitem.h \
    sym/symbolpreviewgraphicsitem.h \

FORMS += \

