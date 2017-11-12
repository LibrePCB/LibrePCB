#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbprojecteditor

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../

SOURCES += \
    boardeditor/boardeditor.cpp \
    boardeditor/boardlayersdock.cpp \
    boardeditor/boardlayerstacksetupdialog.cpp \
    boardeditor/boardviapropertiesdialog.cpp \
    boardeditor/fabricationoutputdialog.cpp \
    boardeditor/fsm/bes_adddevice.cpp \
    boardeditor/fsm/bes_addvia.cpp \
    boardeditor/fsm/bes_base.cpp \
    boardeditor/fsm/bes_drawtrace.cpp \
    boardeditor/fsm/bes_fsm.cpp \
    boardeditor/fsm/bes_select.cpp \
    boardeditor/fsm/boardeditorevent.cpp \
    boardeditor/unplacedcomponentsdock.cpp \
    cmd/cmdaddcomponenttocircuit.cpp \
    cmd/cmdadddevicetoboard.cpp \
    cmd/cmdaddsymboltoschematic.cpp \
    cmd/cmdchangenetsignalofschematicnetsegment.cpp \
    cmd/cmdcombineallitemsunderboardnetpoint.cpp \
    cmd/cmdcombineallitemsunderschematicnetpoint.cpp \
    cmd/cmdcombineboardnetpoints.cpp \
    cmd/cmdcombinenetsignals.cpp \
    cmd/cmdcombineschematicnetpoints.cpp \
    cmd/cmdcombineschematicnetsegments.cpp \
    cmd/cmddetachboardnetpointfromviaorpad.cpp \
    cmd/cmdflipselectedboarditems.cpp \
    cmd/cmdmoveselectedboarditems.cpp \
    cmd/cmdmoveselectedschematicitems.cpp \
    cmd/cmdplaceboardnetpoint.cpp \
    cmd/cmdplaceschematicnetpoint.cpp \
    cmd/cmdremovedevicefromboard.cpp \
    cmd/cmdremoveselectedboarditems.cpp \
    cmd/cmdremoveselectedschematicitems.cpp \
    cmd/cmdremoveunusednetsignals.cpp \
    cmd/cmdremoveviafromboard.cpp \
    cmd/cmdreplacedevice.cpp \
    cmd/cmdrotateselectedboarditems.cpp \
    cmd/cmdrotateselectedschematicitems.cpp \
    dialogs/addcomponentdialog.cpp \
    dialogs/editnetclassesdialog.cpp \
    dialogs/projectpropertieseditordialog.cpp \
    dialogs/projectsettingsdialog.cpp \
    docks/ercmsgdock.cpp \
    newprojectwizard/newprojectwizard.cpp \
    newprojectwizard/newprojectwizardpage_initialization.cpp \
    newprojectwizard/newprojectwizardpage_metadata.cpp \
    newprojectwizard/newprojectwizardpage_versioncontrol.cpp \
    projecteditor.cpp \
    schematiceditor/fsm/schematiceditorevent.cpp \
    schematiceditor/fsm/ses_addcomponent.cpp \
    schematiceditor/fsm/ses_addnetlabel.cpp \
    schematiceditor/fsm/ses_base.cpp \
    schematiceditor/fsm/ses_drawwire.cpp \
    schematiceditor/fsm/ses_fsm.cpp \
    schematiceditor/fsm/ses_select.cpp \
    schematiceditor/schematicclipboard.cpp \
    schematiceditor/schematiceditor.cpp \
    schematiceditor/schematicpagesdock.cpp \
    schematiceditor/symbolinstancepropertiesdialog.cpp \

HEADERS += \
    boardeditor/boardeditor.h \
    boardeditor/boardlayersdock.h \
    boardeditor/boardlayerstacksetupdialog.h \
    boardeditor/boardviapropertiesdialog.h \
    boardeditor/fabricationoutputdialog.h \
    boardeditor/fsm/bes_adddevice.h \
    boardeditor/fsm/bes_addvia.h \
    boardeditor/fsm/bes_base.h \
    boardeditor/fsm/bes_drawtrace.h \
    boardeditor/fsm/bes_fsm.h \
    boardeditor/fsm/bes_select.h \
    boardeditor/fsm/boardeditorevent.h \
    boardeditor/unplacedcomponentsdock.h \
    cmd/cmdaddcomponenttocircuit.h \
    cmd/cmdadddevicetoboard.h \
    cmd/cmdaddsymboltoschematic.h \
    cmd/cmdchangenetsignalofschematicnetsegment.h \
    cmd/cmdcombineallitemsunderboardnetpoint.h \
    cmd/cmdcombineallitemsunderschematicnetpoint.h \
    cmd/cmdcombineboardnetpoints.h \
    cmd/cmdcombinenetsignals.h \
    cmd/cmdcombineschematicnetpoints.h \
    cmd/cmdcombineschematicnetsegments.h \
    cmd/cmddetachboardnetpointfromviaorpad.h \
    cmd/cmdflipselectedboarditems.h \
    cmd/cmdmoveselectedboarditems.h \
    cmd/cmdmoveselectedschematicitems.h \
    cmd/cmdplaceboardnetpoint.h \
    cmd/cmdplaceschematicnetpoint.h \
    cmd/cmdremovedevicefromboard.h \
    cmd/cmdremoveselectedboarditems.h \
    cmd/cmdremoveselectedschematicitems.h \
    cmd/cmdremoveunusednetsignals.h \
    cmd/cmdremoveviafromboard.h \
    cmd/cmdreplacedevice.h \
    cmd/cmdrotateselectedboarditems.h \
    cmd/cmdrotateselectedschematicitems.h \
    dialogs/addcomponentdialog.h \
    dialogs/editnetclassesdialog.h \
    dialogs/projectpropertieseditordialog.h \
    dialogs/projectsettingsdialog.h \
    docks/ercmsgdock.h \
    newprojectwizard/newprojectwizard.h \
    newprojectwizard/newprojectwizardpage_initialization.h \
    newprojectwizard/newprojectwizardpage_metadata.h \
    newprojectwizard/newprojectwizardpage_versioncontrol.h \
    projecteditor.h \
    schematiceditor/fsm/schematiceditorevent.h \
    schematiceditor/fsm/ses_addcomponent.h \
    schematiceditor/fsm/ses_addnetlabel.h \
    schematiceditor/fsm/ses_base.h \
    schematiceditor/fsm/ses_drawwire.h \
    schematiceditor/fsm/ses_fsm.h \
    schematiceditor/fsm/ses_select.h \
    schematiceditor/schematicclipboard.h \
    schematiceditor/schematiceditor.h \
    schematiceditor/schematicpagesdock.h \
    schematiceditor/symbolinstancepropertiesdialog.h \

FORMS += \
    boardeditor/boardeditor.ui \
    boardeditor/boardlayersdock.ui \
    boardeditor/boardlayerstacksetupdialog.ui \
    boardeditor/boardviapropertiesdialog.ui \
    boardeditor/fabricationoutputdialog.ui \
    boardeditor/unplacedcomponentsdock.ui \
    dialogs/addcomponentdialog.ui \
    dialogs/editnetclassesdialog.ui \
    dialogs/projectpropertieseditordialog.ui \
    dialogs/projectsettingsdialog.ui \
    docks/ercmsgdock.ui \
    newprojectwizard/newprojectwizard.ui \
    newprojectwizard/newprojectwizardpage_initialization.ui \
    newprojectwizard/newprojectwizardpage_metadata.ui \
    newprojectwizard/newprojectwizardpage_versioncontrol.ui \
    schematiceditor/schematiceditor.ui \
    schematiceditor/schematicpagesdock.ui \
    schematiceditor/symbolinstancepropertiesdialog.ui \

