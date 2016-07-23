#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbprojecteditor

# Set the path for the generated binary
GENERATED_DIR = ../../generated

# Use common project definitions
include(../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../

SOURCES += \
    projecteditor.cpp \
    schematiceditor/schematiceditor.cpp \
    schematiceditor/schematicpagesdock.cpp \
    schematiceditor/fsm/schematiceditorevent.cpp \
    schematiceditor/fsm/ses_select.cpp \
    schematiceditor/fsm/ses_move.cpp \
    schematiceditor/fsm/ses_drawtext.cpp \
    schematiceditor/fsm/ses_drawrect.cpp \
    schematiceditor/fsm/ses_drawpolygon.cpp \
    schematiceditor/fsm/ses_drawcircle.cpp \
    schematiceditor/fsm/ses_drawellipse.cpp \
    schematiceditor/fsm/ses_drawwire.cpp \
    schematiceditor/fsm/ses_addcomponents.cpp \
    schematiceditor/fsm/ses_base.cpp \
    schematiceditor/fsm/ses_fsm.cpp \
    schematiceditor/symbolinstancepropertiesdialog.cpp \
    schematiceditor/fsm/ses_addnetlabel.cpp \
    schematiceditor/schematicclipboard.cpp \
    boardeditor/boardeditor.cpp \
    boardeditor/unplacedcomponentsdock.cpp \
    boardeditor/fsm/bes_base.cpp \
    boardeditor/fsm/boardeditorevent.cpp \
    boardeditor/fsm/bes_fsm.cpp \
    boardeditor/fsm/bes_select.cpp \
    dialogs/editnetclassesdialog.cpp \
    dialogs/projectpropertieseditordialog.cpp \
    dialogs/projectsettingsdialog.cpp \
    docks/ercmsgdock.cpp \
    dialogs/addcomponentdialog.cpp \
    boardeditor/boardlayersdock.cpp \
    cmd/cmdaddcomponenttocircuit.cpp \
    cmd/cmdaddsymboltoschematic.cpp \
    cmd/cmdadddevicetoboard.cpp \
    cmd/cmdcombinenetsignals.cpp \
    cmd/cmdremoveselectedschematicitems.cpp \
    cmd/cmdcombineschematicnetpoints.cpp \
    cmd/cmdrotateselectedschematicitems.cpp \
    cmd/cmdmoveselectedschematicitems.cpp \
    cmd/cmdmoveselectedboarditems.cpp \
    cmd/cmdrotateselectedboarditems.cpp \
    cmd/cmdflipselectedboarditems.cpp \
    cmd/cmdremoveselectedboarditems.cpp \
    cmd/cmdreplacedevice.cpp \
    cmd/cmdplaceschematicnetpoint.cpp \
    cmd/cmdcombineallnetsignalsunderschematicnetpoint.cpp \
    cmd/cmdremoveunusednetsignals.cpp \
    boardeditor/fsm/bes_drawtrace.cpp \
    boardeditor/fsm/bes_addvia.cpp \
    cmd/cmdplaceboardnetpoint.cpp \
    cmd/cmdcombineboardnetpoints.cpp \
    cmd/cmdcombineallitemsunderboardnetpoint.cpp \
    boardeditor/boardviapropertiesdialog.cpp \
    boardeditor/fsm/bes_adddevice.cpp \
    boardeditor/fabricationoutputdialog.cpp \
    cmd/cmdremovedevicefromboard.cpp \
    cmd/cmdremoveviafromboard.cpp \
    cmd/cmddetachboardnetpointfromviaorpad.cpp

HEADERS += \
    projecteditor.h \
    schematiceditor/schematiceditor.h \
    schematiceditor/schematicpagesdock.h \
    schematiceditor/fsm/schematiceditorevent.h \
    schematiceditor/fsm/ses_select.h \
    schematiceditor/fsm/ses_move.h \
    schematiceditor/fsm/ses_drawtext.h \
    schematiceditor/fsm/ses_drawrect.h \
    schematiceditor/fsm/ses_drawpolygon.h \
    schematiceditor/fsm/ses_drawcircle.h \
    schematiceditor/fsm/ses_drawellipse.h \
    schematiceditor/fsm/ses_drawwire.h \
    schematiceditor/fsm/ses_addcomponents.h \
    schematiceditor/fsm/ses_base.h \
    schematiceditor/fsm/ses_fsm.h \
    schematiceditor/symbolinstancepropertiesdialog.h \
    schematiceditor/fsm/ses_addnetlabel.h \
    schematiceditor/schematicclipboard.h \
    boardeditor/boardeditor.h \
    boardeditor/unplacedcomponentsdock.h \
    boardeditor/fsm/bes_base.h \
    boardeditor/fsm/boardeditorevent.h \
    boardeditor/fsm/bes_fsm.h \
    boardeditor/fsm/bes_select.h \
    dialogs/editnetclassesdialog.h \
    dialogs/projectpropertieseditordialog.h \
    dialogs/projectsettingsdialog.h \
    docks/ercmsgdock.h \
    dialogs/addcomponentdialog.h \
    boardeditor/boardlayersdock.h \
    cmd/cmdaddcomponenttocircuit.h \
    cmd/cmdaddsymboltoschematic.h \
    cmd/cmdadddevicetoboard.h \
    cmd/cmdcombinenetsignals.h \
    cmd/cmdremoveselectedschematicitems.h \
    cmd/cmdcombineschematicnetpoints.h \
    cmd/cmdrotateselectedschematicitems.h \
    cmd/cmdmoveselectedschematicitems.h \
    cmd/cmdmoveselectedboarditems.h \
    cmd/cmdrotateselectedboarditems.h \
    cmd/cmdflipselectedboarditems.h \
    cmd/cmdremoveselectedboarditems.h \
    cmd/cmdreplacedevice.h \
    cmd/cmdplaceschematicnetpoint.h \
    cmd/cmdcombineallnetsignalsunderschematicnetpoint.h \
    cmd/cmdremoveunusednetsignals.h \
    boardeditor/fsm/bes_drawtrace.h \
    boardeditor/fsm/bes_addvia.h \
    cmd/cmdplaceboardnetpoint.h \
    cmd/cmdcombineboardnetpoints.h \
    cmd/cmdcombineallitemsunderboardnetpoint.h \
    boardeditor/boardviapropertiesdialog.h \
    boardeditor/fsm/bes_adddevice.h \
    boardeditor/fabricationoutputdialog.h \
    cmd/cmdremovedevicefromboard.h \
    cmd/cmdremoveviafromboard.h \
    cmd/cmddetachboardnetpointfromviaorpad.h

FORMS += \
    schematiceditor/schematiceditor.ui \
    schematiceditor/schematicpagesdock.ui \
    schematiceditor/symbolinstancepropertiesdialog.ui \
    boardeditor/boardeditor.ui \
    boardeditor/unplacedcomponentsdock.ui \
    dialogs/editnetclassesdialog.ui \
    dialogs/projectpropertieseditordialog.ui \
    dialogs/projectsettingsdialog.ui \
    docks/ercmsgdock.ui \
    dialogs/addcomponentdialog.ui \
    boardeditor/boardlayersdock.ui \
    boardeditor/boardviapropertiesdialog.ui \
    boardeditor/fabricationoutputdialog.ui

