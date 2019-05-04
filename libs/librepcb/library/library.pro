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
    cat/cmd/cmdlibrarycategoryedit.cpp \
    cat/componentcategory.cpp \
    cat/librarycategory.cpp \
    cat/packagecategory.cpp \
    cmd/cmdlibrarybaseelementedit.cpp \
    cmd/cmdlibraryedit.cpp \
    cmd/cmdlibraryelementedit.cpp \
    cmp/cmd/cmdcomponentedit.cpp \
    cmp/cmd/cmdcomponentpinsignalmapitemedit.cpp \
    cmp/cmd/cmdcomponentsignaledit.cpp \
    cmp/cmd/cmdcomponentsymbolvariantedit.cpp \
    cmp/cmd/cmdcomponentsymbolvariantitemedit.cpp \
    cmp/cmpsigpindisplaytype.cpp \
    cmp/component.cpp \
    cmp/componentcheck.cpp \
    cmp/componentpinsignalmap.cpp \
    cmp/componentpinsignalmapmodel.cpp \
    cmp/componentsignal.cpp \
    cmp/componentsignallistmodel.cpp \
    cmp/componentsymbolvariant.cpp \
    cmp/componentsymbolvariantitem.cpp \
    cmp/componentsymbolvariantitemlistmodel.cpp \
    cmp/componentsymbolvariantlistmodel.cpp \
    cmp/msg/msgduplicatesignalname.cpp \
    cmp/msg/msgmissingcomponentdefaultvalue.cpp \
    cmp/msg/msgmissingcomponentprefix.cpp \
    cmp/msg/msgmissingsymbolvariant.cpp \
    cmp/msg/msgmissingsymbolvariantitem.cpp \
    dev/cmd/cmddeviceedit.cpp \
    dev/cmd/cmddevicepadsignalmapitemedit.cpp \
    dev/device.cpp \
    dev/devicepadsignalmap.cpp \
    dev/devicepadsignalmapmodel.cpp \
    library.cpp \
    librarybaseelement.cpp \
    librarybaseelementcheck.cpp \
    libraryelement.cpp \
    libraryelementcache.cpp \
    libraryelementcheck.cpp \
    msg/libraryelementcheckmessage.cpp \
    msg/msgmissingauthor.cpp \
    msg/msgmissingcategories.cpp \
    msg/msgnamenottitlecase.cpp \
    pkg/cmd/cmdfootprintedit.cpp \
    pkg/cmd/cmdfootprintpadedit.cpp \
    pkg/cmd/cmdpackagepadedit.cpp \
    pkg/footprint.cpp \
    pkg/footprintgraphicsitem.cpp \
    pkg/footprintpad.cpp \
    pkg/footprintpadgraphicsitem.cpp \
    pkg/footprintpadpreviewgraphicsitem.cpp \
    pkg/footprintpreviewgraphicsitem.cpp \
    pkg/msg/msgduplicatepadname.cpp \
    pkg/msg/msgmissingfootprint.cpp \
    pkg/msg/msgmissingfootprintname.cpp \
    pkg/msg/msgmissingfootprintvalue.cpp \
    pkg/msg/msgpadoverlapswithplacement.cpp \
    pkg/msg/msgwrongfootprinttextlayer.cpp \
    pkg/package.cpp \
    pkg/packagecheck.cpp \
    pkg/packagepad.cpp \
    sym/cmd/cmdsymbolpinedit.cpp \
    sym/msg/msgduplicatepinname.cpp \
    sym/msg/msgmissingsymbolname.cpp \
    sym/msg/msgmissingsymbolvalue.cpp \
    sym/msg/msgoverlappingsymbolpins.cpp \
    sym/msg/msgsymbolpinnotongrid.cpp \
    sym/msg/msgwrongsymboltextlayer.cpp \
    sym/symbol.cpp \
    sym/symbolcheck.cpp \
    sym/symbolgraphicsitem.cpp \
    sym/symbolpin.cpp \
    sym/symbolpingraphicsitem.cpp \
    sym/symbolpinpreviewgraphicsitem.cpp \
    sym/symbolpreviewgraphicsitem.cpp \

HEADERS += \
    cat/cmd/cmdlibrarycategoryedit.h \
    cat/componentcategory.h \
    cat/librarycategory.h \
    cat/packagecategory.h \
    cmd/cmdlibrarybaseelementedit.h \
    cmd/cmdlibraryedit.h \
    cmd/cmdlibraryelementedit.h \
    cmp/cmd/cmdcomponentedit.h \
    cmp/cmd/cmdcomponentpinsignalmapitemedit.h \
    cmp/cmd/cmdcomponentsignaledit.h \
    cmp/cmd/cmdcomponentsymbolvariantedit.h \
    cmp/cmd/cmdcomponentsymbolvariantitemedit.h \
    cmp/cmpsigpindisplaytype.h \
    cmp/component.h \
    cmp/componentcheck.h \
    cmp/componentpinsignalmap.h \
    cmp/componentpinsignalmapmodel.h \
    cmp/componentprefix.h \
    cmp/componentsignal.h \
    cmp/componentsignallistmodel.h \
    cmp/componentsymbolvariant.h \
    cmp/componentsymbolvariantitem.h \
    cmp/componentsymbolvariantitemlistmodel.h \
    cmp/componentsymbolvariantitemsuffix.h \
    cmp/componentsymbolvariantlistmodel.h \
    cmp/msg/msgduplicatesignalname.h \
    cmp/msg/msgmissingcomponentdefaultvalue.h \
    cmp/msg/msgmissingcomponentprefix.h \
    cmp/msg/msgmissingsymbolvariant.h \
    cmp/msg/msgmissingsymbolvariantitem.h \
    dev/cmd/cmddeviceedit.h \
    dev/cmd/cmddevicepadsignalmapitemedit.h \
    dev/device.h \
    dev/devicepadsignalmap.h \
    dev/devicepadsignalmapmodel.h \
    elements.h \
    library.h \
    librarybaseelement.h \
    librarybaseelementcheck.h \
    libraryelement.h \
    libraryelementcache.h \
    libraryelementcheck.h \
    msg/libraryelementcheckmessage.h \
    msg/msgmissingauthor.h \
    msg/msgmissingcategories.h \
    msg/msgnamenottitlecase.h \
    pkg/cmd/cmdfootprintedit.h \
    pkg/cmd/cmdfootprintpadedit.h \
    pkg/cmd/cmdpackagepadedit.h \
    pkg/footprint.h \
    pkg/footprintgraphicsitem.h \
    pkg/footprintpad.h \
    pkg/footprintpadgraphicsitem.h \
    pkg/footprintpadpreviewgraphicsitem.h \
    pkg/footprintpreviewgraphicsitem.h \
    pkg/msg/msgduplicatepadname.h \
    pkg/msg/msgmissingfootprint.h \
    pkg/msg/msgmissingfootprintname.h \
    pkg/msg/msgmissingfootprintvalue.h \
    pkg/msg/msgpadoverlapswithplacement.h \
    pkg/msg/msgwrongfootprinttextlayer.h \
    pkg/package.h \
    pkg/packagecheck.h \
    pkg/packagepad.h \
    sym/cmd/cmdsymbolpinedit.h \
    sym/msg/msgduplicatepinname.h \
    sym/msg/msgmissingsymbolname.h \
    sym/msg/msgmissingsymbolvalue.h \
    sym/msg/msgoverlappingsymbolpins.h \
    sym/msg/msgsymbolpinnotongrid.h \
    sym/msg/msgwrongsymboltextlayer.h \
    sym/symbol.h \
    sym/symbolcheck.h \
    sym/symbolgraphicsitem.h \
    sym/symbolpin.h \
    sym/symbolpingraphicsitem.h \
    sym/symbolpinpreviewgraphicsitem.h \
    sym/symbolpreviewgraphicsitem.h \

FORMS += \

