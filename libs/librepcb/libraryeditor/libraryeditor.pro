#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibraryeditor

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../

SOURCES += \
    cmp/cmpsigpindisplaytypecombobox.cpp \
    cmp/componenteditorwidget.cpp \
    cmp/componentsignallisteditorwidget.cpp \
    cmp/componentsymbolvarianteditdialog.cpp \
    cmp/componentsymbolvariantitemlisteditorwidget.cpp \
    cmp/componentsymbolvariantlistwidget.cpp \
    cmp/compsymbvarpinsignalmapeditorwidget.cpp \
    cmpcat/componentcategoryeditorwidget.cpp \
    common/categorychooserdialog.cpp \
    common/categorylisteditorwidget.cpp \
    common/categorytreelabeltextbuilder.cpp \
    common/componentchooserdialog.cpp \
    common/editorwidgetbase.cpp \
    common/packagechooserdialog.cpp \
    common/symbolchooserdialog.cpp \
    dev/deviceeditorwidget.cpp \
    dev/padsignalmapeditorwidget.cpp \
    lib/librarylisteditorwidget.cpp \
    lib/libraryoverviewwidget.cpp \
    libraryeditor.cpp \
    pkg/dialogs/footprintpadpropertiesdialog.cpp \
    pkg/footprintlisteditorwidget.cpp \
    pkg/fsm/cmd/cmdmoveselectedfootprintitems.cpp \
    pkg/fsm/cmd/cmdremoveselectedfootprintitems.cpp \
    pkg/fsm/cmd/cmdrotateselectedfootprintitems.cpp \
    pkg/fsm/packageeditorfsm.cpp \
    pkg/fsm/packageeditorstate.cpp \
    pkg/fsm/packageeditorstate_addholes.cpp \
    pkg/fsm/packageeditorstate_addnames.cpp \
    pkg/fsm/packageeditorstate_addpads.cpp \
    pkg/fsm/packageeditorstate_addvalues.cpp \
    pkg/fsm/packageeditorstate_drawellipse.cpp \
    pkg/fsm/packageeditorstate_drawline.cpp \
    pkg/fsm/packageeditorstate_drawpolygon.cpp \
    pkg/fsm/packageeditorstate_drawpolygonbase.cpp \
    pkg/fsm/packageeditorstate_drawrect.cpp \
    pkg/fsm/packageeditorstate_drawtext.cpp \
    pkg/fsm/packageeditorstate_drawtextbase.cpp \
    pkg/fsm/packageeditorstate_select.cpp \
    pkg/packageeditorwidget.cpp \
    pkg/packagepadlisteditorwidget.cpp \
    pkg/widgets/boardsideselectorwidget.cpp \
    pkg/widgets/footprintpadshapeselectorwidget.cpp \
    pkg/widgets/packagepadcombobox.cpp \
    pkgcat/packagecategoryeditorwidget.cpp \
    sym/dialogs/symbolpinpropertiesdialog.cpp \
    sym/fsm/cmd/cmdmoveselectedsymbolitems.cpp \
    sym/fsm/cmd/cmdremoveselectedsymbolitems.cpp \
    sym/fsm/cmd/cmdrotateselectedsymbolitems.cpp \
    sym/fsm/symboleditorfsm.cpp \
    sym/fsm/symboleditorstate.cpp \
    sym/fsm/symboleditorstate_addnames.cpp \
    sym/fsm/symboleditorstate_addpins.cpp \
    sym/fsm/symboleditorstate_addvalues.cpp \
    sym/fsm/symboleditorstate_drawellipse.cpp \
    sym/fsm/symboleditorstate_drawline.cpp \
    sym/fsm/symboleditorstate_drawpolygon.cpp \
    sym/fsm/symboleditorstate_drawpolygonbase.cpp \
    sym/fsm/symboleditorstate_drawrect.cpp \
    sym/fsm/symboleditorstate_drawtext.cpp \
    sym/fsm/symboleditorstate_drawtextbase.cpp \
    sym/fsm/symboleditorstate_select.cpp \
    sym/symboleditorwidget.cpp \

HEADERS += \
    cmp/cmpsigpindisplaytypecombobox.h \
    cmp/componenteditorwidget.h \
    cmp/componentsignallisteditorwidget.h \
    cmp/componentsymbolvarianteditdialog.h \
    cmp/componentsymbolvariantitemlisteditorwidget.h \
    cmp/componentsymbolvariantlistwidget.h \
    cmp/compsymbvarpinsignalmapeditorwidget.h \
    cmp/if_componentsymbolvarianteditorprovider.h \
    cmpcat/componentcategoryeditorwidget.h \
    common/categorychooserdialog.h \
    common/categorylisteditorwidget.h \
    common/categorytreelabeltextbuilder.h \
    common/componentchooserdialog.h \
    common/editorwidgetbase.h \
    common/packagechooserdialog.h \
    common/symbolchooserdialog.h \
    dev/deviceeditorwidget.h \
    dev/padsignalmapeditorwidget.h \
    lib/librarylisteditorwidget.h \
    lib/libraryoverviewwidget.h \
    libraryeditor.h \
    pkg/dialogs/footprintpadpropertiesdialog.h \
    pkg/footprintlisteditorwidget.h \
    pkg/fsm/cmd/cmdmoveselectedfootprintitems.h \
    pkg/fsm/cmd/cmdremoveselectedfootprintitems.h \
    pkg/fsm/cmd/cmdrotateselectedfootprintitems.h \
    pkg/fsm/packageeditorfsm.h \
    pkg/fsm/packageeditorstate.h \
    pkg/fsm/packageeditorstate_addholes.h \
    pkg/fsm/packageeditorstate_addnames.h \
    pkg/fsm/packageeditorstate_addpads.h \
    pkg/fsm/packageeditorstate_addvalues.h \
    pkg/fsm/packageeditorstate_drawellipse.h \
    pkg/fsm/packageeditorstate_drawline.h \
    pkg/fsm/packageeditorstate_drawpolygon.h \
    pkg/fsm/packageeditorstate_drawpolygonbase.h \
    pkg/fsm/packageeditorstate_drawrect.h \
    pkg/fsm/packageeditorstate_drawtext.h \
    pkg/fsm/packageeditorstate_drawtextbase.h \
    pkg/fsm/packageeditorstate_select.h \
    pkg/packageeditorwidget.h \
    pkg/packagepadlisteditorwidget.h \
    pkg/widgets/boardsideselectorwidget.h \
    pkg/widgets/footprintpadshapeselectorwidget.h \
    pkg/widgets/packagepadcombobox.h \
    pkgcat/packagecategoryeditorwidget.h \
    sym/dialogs/symbolpinpropertiesdialog.h \
    sym/fsm/cmd/cmdmoveselectedsymbolitems.h \
    sym/fsm/cmd/cmdremoveselectedsymbolitems.h \
    sym/fsm/cmd/cmdrotateselectedsymbolitems.h \
    sym/fsm/symboleditorfsm.h \
    sym/fsm/symboleditorstate.h \
    sym/fsm/symboleditorstate_addnames.h \
    sym/fsm/symboleditorstate_addpins.h \
    sym/fsm/symboleditorstate_addvalues.h \
    sym/fsm/symboleditorstate_drawellipse.h \
    sym/fsm/symboleditorstate_drawline.h \
    sym/fsm/symboleditorstate_drawpolygon.h \
    sym/fsm/symboleditorstate_drawpolygonbase.h \
    sym/fsm/symboleditorstate_drawrect.h \
    sym/fsm/symboleditorstate_drawtext.h \
    sym/fsm/symboleditorstate_drawtextbase.h \
    sym/fsm/symboleditorstate_select.h \
    sym/symboleditorwidget.h \

FORMS += \
    cmp/componenteditorwidget.ui \
    cmp/componentsymbolvarianteditdialog.ui \
    cmpcat/componentcategoryeditorwidget.ui \
    common/categorychooserdialog.ui \
    common/categorylisteditorwidget.ui \
    common/componentchooserdialog.ui \
    common/packagechooserdialog.ui \
    common/symbolchooserdialog.ui \
    dev/deviceeditorwidget.ui \
    lib/librarylisteditorwidget.ui \
    lib/libraryoverviewwidget.ui \
    libraryeditor.ui \
    pkg/dialogs/footprintpadpropertiesdialog.ui \
    pkg/packageeditorwidget.ui \
    pkgcat/packagecategoryeditorwidget.ui \
    sym/dialogs/symbolpinpropertiesdialog.ui \
    sym/symboleditorwidget.ui \

