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
    cmpcat/componentcategoryeditorwidget.cpp \
    common/categorychooserdialog.cpp \
    common/categorylisteditorwidget.cpp \
    common/categorytreelabeltextbuilder.cpp \
    common/editorwidgetbase.cpp \
    libraryeditor.cpp \
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
    cmpcat/componentcategoryeditorwidget.h \
    common/categorychooserdialog.h \
    common/categorylisteditorwidget.h \
    common/categorytreelabeltextbuilder.h \
    common/editorwidgetbase.h \
    libraryeditor.h \
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
    cmpcat/componentcategoryeditorwidget.ui \
    common/categorychooserdialog.ui \
    common/categorylisteditorwidget.ui \
    libraryeditor.ui \
    pkgcat/packagecategoryeditorwidget.ui \
    sym/dialogs/symbolpinpropertiesdialog.ui \
    sym/symboleditorwidget.ui \

