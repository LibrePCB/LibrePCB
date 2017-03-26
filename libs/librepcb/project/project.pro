#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:53:17
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbproject

# Set the path for the generated binary
GENERATED_DIR = ../../../generated

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

CONFIG += staticlib

INCLUDEPATH += \
    ../../

SOURCES += \
    boards/board.cpp \
    boards/boardgerberexport.cpp \
    boards/boardlayerstack.cpp \
    boards/cmd/cmdboardadd.cpp \
    boards/cmd/cmdboarddesignrulesmodify.cpp \
    boards/cmd/cmdboardnetlineadd.cpp \
    boards/cmd/cmdboardnetlineremove.cpp \
    boards/cmd/cmdboardnetpointadd.cpp \
    boards/cmd/cmdboardnetpointedit.cpp \
    boards/cmd/cmdboardnetpointremove.cpp \
    boards/cmd/cmdboardviaadd.cpp \
    boards/cmd/cmdboardviaedit.cpp \
    boards/cmd/cmdboardviaremove.cpp \
    boards/cmd/cmddeviceinstanceadd.cpp \
    boards/cmd/cmddeviceinstanceedit.cpp \
    boards/cmd/cmddeviceinstanceremove.cpp \
    boards/graphicsitems/bgi_base.cpp \
    boards/graphicsitems/bgi_footprint.cpp \
    boards/graphicsitems/bgi_footprintpad.cpp \
    boards/graphicsitems/bgi_netline.cpp \
    boards/graphicsitems/bgi_netpoint.cpp \
    boards/graphicsitems/bgi_polygon.cpp \
    boards/graphicsitems/bgi_via.cpp \
    boards/items/bi_base.cpp \
    boards/items/bi_device.cpp \
    boards/items/bi_footprint.cpp \
    boards/items/bi_footprintpad.cpp \
    boards/items/bi_netline.cpp \
    boards/items/bi_netpoint.cpp \
    boards/items/bi_polygon.cpp \
    boards/items/bi_via.cpp \
    circuit/circuit.cpp \
    circuit/cmd/cmdcomponentinstanceadd.cpp \
    circuit/cmd/cmdcomponentinstanceedit.cpp \
    circuit/cmd/cmdcomponentinstanceremove.cpp \
    circuit/cmd/cmdcompsiginstsetnetsignal.cpp \
    circuit/cmd/cmdnetclassadd.cpp \
    circuit/cmd/cmdnetclassedit.cpp \
    circuit/cmd/cmdnetclassremove.cpp \
    circuit/cmd/cmdnetsignaladd.cpp \
    circuit/cmd/cmdnetsignaledit.cpp \
    circuit/cmd/cmdnetsignalremove.cpp \
    circuit/componentinstance.cpp \
    circuit/componentsignalinstance.cpp \
    circuit/netclass.cpp \
    circuit/netsignal.cpp \
    cmd/cmdprojectsetmetadata.cpp \
    erc/ercmsg.cpp \
    erc/ercmsglist.cpp \
    library/cmd/cmdprojectlibraryaddelement.cpp \
    library/projectlibrary.cpp \
    project.cpp \
    schematics/cmd/cmdschematicadd.cpp \
    schematics/cmd/cmdschematicnetlabeladd.cpp \
    schematics/cmd/cmdschematicnetlabeledit.cpp \
    schematics/cmd/cmdschematicnetlabelremove.cpp \
    schematics/cmd/cmdschematicnetlineadd.cpp \
    schematics/cmd/cmdschematicnetlineremove.cpp \
    schematics/cmd/cmdschematicnetpointadd.cpp \
    schematics/cmd/cmdschematicnetpointedit.cpp \
    schematics/cmd/cmdschematicnetpointremove.cpp \
    schematics/cmd/cmdschematicremove.cpp \
    schematics/cmd/cmdsymbolinstanceadd.cpp \
    schematics/cmd/cmdsymbolinstanceedit.cpp \
    schematics/cmd/cmdsymbolinstanceremove.cpp \
    schematics/graphicsitems/sgi_base.cpp \
    schematics/graphicsitems/sgi_netlabel.cpp \
    schematics/graphicsitems/sgi_netline.cpp \
    schematics/graphicsitems/sgi_netpoint.cpp \
    schematics/graphicsitems/sgi_symbol.cpp \
    schematics/graphicsitems/sgi_symbolpin.cpp \
    schematics/items/si_base.cpp \
    schematics/items/si_netlabel.cpp \
    schematics/items/si_netline.cpp \
    schematics/items/si_netpoint.cpp \
    schematics/items/si_symbol.cpp \
    schematics/items/si_symbolpin.cpp \
    schematics/schematic.cpp \
    schematics/schematiclayerprovider.cpp \
    settings/cmd/cmdprojectsettingschange.cpp \
    settings/projectsettings.cpp \

HEADERS += \
    boards/board.h \
    boards/boardgerberexport.h \
    boards/boardlayerstack.h \
    boards/cmd/cmdboardadd.h \
    boards/cmd/cmdboarddesignrulesmodify.h \
    boards/cmd/cmdboardnetlineadd.h \
    boards/cmd/cmdboardnetlineremove.h \
    boards/cmd/cmdboardnetpointadd.h \
    boards/cmd/cmdboardnetpointedit.h \
    boards/cmd/cmdboardnetpointremove.h \
    boards/cmd/cmdboardviaadd.h \
    boards/cmd/cmdboardviaedit.h \
    boards/cmd/cmdboardviaremove.h \
    boards/cmd/cmddeviceinstanceadd.h \
    boards/cmd/cmddeviceinstanceedit.h \
    boards/cmd/cmddeviceinstanceremove.h \
    boards/graphicsitems/bgi_base.h \
    boards/graphicsitems/bgi_footprint.h \
    boards/graphicsitems/bgi_footprintpad.h \
    boards/graphicsitems/bgi_netline.h \
    boards/graphicsitems/bgi_netpoint.h \
    boards/graphicsitems/bgi_polygon.h \
    boards/graphicsitems/bgi_via.h \
    boards/items/bi_base.h \
    boards/items/bi_device.h \
    boards/items/bi_footprint.h \
    boards/items/bi_footprintpad.h \
    boards/items/bi_netline.h \
    boards/items/bi_netpoint.h \
    boards/items/bi_polygon.h \
    boards/items/bi_via.h \
    circuit/circuit.h \
    circuit/cmd/cmdcomponentinstanceadd.h \
    circuit/cmd/cmdcomponentinstanceedit.h \
    circuit/cmd/cmdcomponentinstanceremove.h \
    circuit/cmd/cmdcompsiginstsetnetsignal.h \
    circuit/cmd/cmdnetclassadd.h \
    circuit/cmd/cmdnetclassedit.h \
    circuit/cmd/cmdnetclassremove.h \
    circuit/cmd/cmdnetsignaladd.h \
    circuit/cmd/cmdnetsignaledit.h \
    circuit/cmd/cmdnetsignalremove.h \
    circuit/componentinstance.h \
    circuit/componentsignalinstance.h \
    circuit/netclass.h \
    circuit/netsignal.h \
    cmd/cmdprojectsetmetadata.h \
    erc/ercmsg.h \
    erc/ercmsglist.h \
    erc/if_ercmsgprovider.h \
    library/cmd/cmdprojectlibraryaddelement.h \
    library/projectlibrary.h \
    project.h \
    schematics/cmd/cmdschematicadd.h \
    schematics/cmd/cmdschematicnetlabeladd.h \
    schematics/cmd/cmdschematicnetlabeledit.h \
    schematics/cmd/cmdschematicnetlabelremove.h \
    schematics/cmd/cmdschematicnetlineadd.h \
    schematics/cmd/cmdschematicnetlineremove.h \
    schematics/cmd/cmdschematicnetpointadd.h \
    schematics/cmd/cmdschematicnetpointedit.h \
    schematics/cmd/cmdschematicnetpointremove.h \
    schematics/cmd/cmdschematicremove.h \
    schematics/cmd/cmdsymbolinstanceadd.h \
    schematics/cmd/cmdsymbolinstanceedit.h \
    schematics/cmd/cmdsymbolinstanceremove.h \
    schematics/graphicsitems/sgi_base.h \
    schematics/graphicsitems/sgi_netlabel.h \
    schematics/graphicsitems/sgi_netline.h \
    schematics/graphicsitems/sgi_netpoint.h \
    schematics/graphicsitems/sgi_symbol.h \
    schematics/graphicsitems/sgi_symbolpin.h \
    schematics/items/si_base.h \
    schematics/items/si_netlabel.h \
    schematics/items/si_netline.h \
    schematics/items/si_netpoint.h \
    schematics/items/si_symbol.h \
    schematics/items/si_symbolpin.h \
    schematics/schematic.h \
    schematics/schematiclayerprovider.h \
    settings/cmd/cmdprojectsettingschange.h \
    settings/projectsettings.h \

FORMS += \

