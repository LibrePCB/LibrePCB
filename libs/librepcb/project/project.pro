#-------------------------------------------------
# Lib: Classes which represent a LibrePCB project
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbproject

# Use common project definitions
include(../../../common.pri)

QT += core widgets xml sql printsupport

isEmpty(UNBUNDLE) {
    CONFIG += staticlib
} else {
    target.path = $${PREFIX}/lib
    INSTALLS += target
}

INCLUDEPATH += \
    ../../ \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

RESOURCES += \
    ../../../img/images.qrc \

SOURCES += \
    boards/board.cpp \
    boards/boardairwiresbuilder.cpp \
    boards/boardfabricationoutputsettings.cpp \
    boards/boardgerberexport.cpp \
    boards/boardlayerstack.cpp \
    boards/boardpickplacegenerator.cpp \
    boards/boardplanefragmentsbuilder.cpp \
    boards/boardselectionquery.cpp \
    boards/boardusersettings.cpp \
    boards/cmd/cmdboardadd.cpp \
    boards/cmd/cmdboarddesignrulesmodify.cpp \
    boards/cmd/cmdboardholeadd.cpp \
    boards/cmd/cmdboardholeremove.cpp \
    boards/cmd/cmdboardlayerstackedit.cpp \
    boards/cmd/cmdboardnetlineedit.cpp \
    boards/cmd/cmdboardnetpointedit.cpp \
    boards/cmd/cmdboardnetsegmentadd.cpp \
    boards/cmd/cmdboardnetsegmentaddelements.cpp \
    boards/cmd/cmdboardnetsegmentedit.cpp \
    boards/cmd/cmdboardnetsegmentremove.cpp \
    boards/cmd/cmdboardnetsegmentremoveelements.cpp \
    boards/cmd/cmdboardplaneadd.cpp \
    boards/cmd/cmdboardplaneedit.cpp \
    boards/cmd/cmdboardplaneremove.cpp \
    boards/cmd/cmdboardpolygonadd.cpp \
    boards/cmd/cmdboardpolygonremove.cpp \
    boards/cmd/cmdboardremove.cpp \
    boards/cmd/cmdboardstroketextadd.cpp \
    boards/cmd/cmdboardstroketextremove.cpp \
    boards/cmd/cmdboardviaedit.cpp \
    boards/cmd/cmddeviceinstanceadd.cpp \
    boards/cmd/cmddeviceinstanceedit.cpp \
    boards/cmd/cmddeviceinstanceeditall.cpp \
    boards/cmd/cmddeviceinstanceremove.cpp \
    boards/cmd/cmdfootprintstroketextadd.cpp \
    boards/cmd/cmdfootprintstroketextremove.cpp \
    boards/cmd/cmdfootprintstroketextsreset.cpp \
    boards/drc/boardclipperpathgenerator.cpp \
    boards/drc/boarddesignrulecheck.cpp \
    boards/drc/boarddesignrulecheckmessage.cpp \
    boards/graphicsitems/bgi_airwire.cpp \
    boards/graphicsitems/bgi_base.cpp \
    boards/graphicsitems/bgi_footprint.cpp \
    boards/graphicsitems/bgi_footprintpad.cpp \
    boards/graphicsitems/bgi_netline.cpp \
    boards/graphicsitems/bgi_netpoint.cpp \
    boards/graphicsitems/bgi_plane.cpp \
    boards/graphicsitems/bgi_via.cpp \
    boards/items/bi_airwire.cpp \
    boards/items/bi_base.cpp \
    boards/items/bi_device.cpp \
    boards/items/bi_footprint.cpp \
    boards/items/bi_footprintpad.cpp \
    boards/items/bi_hole.cpp \
    boards/items/bi_netline.cpp \
    boards/items/bi_netpoint.cpp \
    boards/items/bi_netsegment.cpp \
    boards/items/bi_plane.cpp \
    boards/items/bi_polygon.cpp \
    boards/items/bi_stroketext.cpp \
    boards/items/bi_via.cpp \
    bomgenerator.cpp \
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
    erc/ercmsg.cpp \
    erc/ercmsglist.cpp \
    library/cmd/cmdprojectlibraryaddelement.cpp \
    library/cmd/cmdprojectlibraryremoveelement.cpp \
    library/projectlibrary.cpp \
    metadata/cmd/cmdprojectmetadataedit.cpp \
    metadata/projectmetadata.cpp \
    project.cpp \
    schematics/cmd/cmdschematicadd.cpp \
    schematics/cmd/cmdschematicedit.cpp \
    schematics/cmd/cmdschematicnetlabeladd.cpp \
    schematics/cmd/cmdschematicnetlabelanchorsupdate.cpp \
    schematics/cmd/cmdschematicnetlabeledit.cpp \
    schematics/cmd/cmdschematicnetlabelremove.cpp \
    schematics/cmd/cmdschematicnetpointedit.cpp \
    schematics/cmd/cmdschematicnetsegmentadd.cpp \
    schematics/cmd/cmdschematicnetsegmentaddelements.cpp \
    schematics/cmd/cmdschematicnetsegmentedit.cpp \
    schematics/cmd/cmdschematicnetsegmentremove.cpp \
    schematics/cmd/cmdschematicnetsegmentremoveelements.cpp \
    schematics/cmd/cmdschematicpolygonadd.cpp \
    schematics/cmd/cmdschematicpolygonremove.cpp \
    schematics/cmd/cmdschematicremove.cpp \
    schematics/cmd/cmdschematictextadd.cpp \
    schematics/cmd/cmdschematictextremove.cpp \
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
    schematics/items/si_netsegment.cpp \
    schematics/items/si_polygon.cpp \
    schematics/items/si_symbol.cpp \
    schematics/items/si_symbolpin.cpp \
    schematics/items/si_text.cpp \
    schematics/schematic.cpp \
    schematics/schematiclayerprovider.cpp \
    schematics/schematicselectionquery.cpp \
    settings/cmd/cmdprojectsettingschange.cpp \
    settings/projectsettings.cpp \

HEADERS += \
    boards/board.h \
    boards/boardairwiresbuilder.h \
    boards/boardfabricationoutputsettings.h \
    boards/boardgerberexport.h \
    boards/boardlayerstack.h \
    boards/boardpickplacegenerator.h \
    boards/boardplanefragmentsbuilder.h \
    boards/boardselectionquery.h \
    boards/boardusersettings.h \
    boards/cmd/cmdboardadd.h \
    boards/cmd/cmdboarddesignrulesmodify.h \
    boards/cmd/cmdboardholeadd.h \
    boards/cmd/cmdboardholeremove.h \
    boards/cmd/cmdboardlayerstackedit.h \
    boards/cmd/cmdboardnetlineedit.h \
    boards/cmd/cmdboardnetpointedit.h \
    boards/cmd/cmdboardnetsegmentadd.h \
    boards/cmd/cmdboardnetsegmentaddelements.h \
    boards/cmd/cmdboardnetsegmentedit.h \
    boards/cmd/cmdboardnetsegmentremove.h \
    boards/cmd/cmdboardnetsegmentremoveelements.h \
    boards/cmd/cmdboardplaneadd.h \
    boards/cmd/cmdboardplaneedit.h \
    boards/cmd/cmdboardplaneremove.h \
    boards/cmd/cmdboardpolygonadd.h \
    boards/cmd/cmdboardpolygonremove.h \
    boards/cmd/cmdboardremove.h \
    boards/cmd/cmdboardstroketextadd.h \
    boards/cmd/cmdboardstroketextremove.h \
    boards/cmd/cmdboardviaedit.h \
    boards/cmd/cmddeviceinstanceadd.h \
    boards/cmd/cmddeviceinstanceedit.h \
    boards/cmd/cmddeviceinstanceeditall.h \
    boards/cmd/cmddeviceinstanceremove.h \
    boards/cmd/cmdfootprintstroketextadd.h \
    boards/cmd/cmdfootprintstroketextremove.h \
    boards/cmd/cmdfootprintstroketextsreset.h \
    boards/drc/boardclipperpathgenerator.h \
    boards/drc/boarddesignrulecheck.h \
    boards/drc/boarddesignrulecheckmessage.h \
    boards/graphicsitems/bgi_airwire.h \
    boards/graphicsitems/bgi_base.h \
    boards/graphicsitems/bgi_footprint.h \
    boards/graphicsitems/bgi_footprintpad.h \
    boards/graphicsitems/bgi_netline.h \
    boards/graphicsitems/bgi_netpoint.h \
    boards/graphicsitems/bgi_plane.h \
    boards/graphicsitems/bgi_via.h \
    boards/items/bi_airwire.h \
    boards/items/bi_base.h \
    boards/items/bi_device.h \
    boards/items/bi_footprint.h \
    boards/items/bi_footprintpad.h \
    boards/items/bi_hole.h \
    boards/items/bi_netline.h \
    boards/items/bi_netpoint.h \
    boards/items/bi_netsegment.h \
    boards/items/bi_plane.h \
    boards/items/bi_polygon.h \
    boards/items/bi_stroketext.h \
    boards/items/bi_via.h \
    bomgenerator.h \
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
    erc/ercmsg.h \
    erc/ercmsglist.h \
    erc/if_ercmsgprovider.h \
    library/cmd/cmdprojectlibraryaddelement.h \
    library/cmd/cmdprojectlibraryremoveelement.h \
    library/projectlibrary.h \
    metadata/cmd/cmdprojectmetadataedit.h \
    metadata/projectmetadata.h \
    project.h \
    schematics/cmd/cmdschematicadd.h \
    schematics/cmd/cmdschematicedit.h \
    schematics/cmd/cmdschematicnetlabeladd.h \
    schematics/cmd/cmdschematicnetlabelanchorsupdate.h \
    schematics/cmd/cmdschematicnetlabeledit.h \
    schematics/cmd/cmdschematicnetlabelremove.h \
    schematics/cmd/cmdschematicnetpointedit.h \
    schematics/cmd/cmdschematicnetsegmentadd.h \
    schematics/cmd/cmdschematicnetsegmentaddelements.h \
    schematics/cmd/cmdschematicnetsegmentedit.h \
    schematics/cmd/cmdschematicnetsegmentremove.h \
    schematics/cmd/cmdschematicnetsegmentremoveelements.h \
    schematics/cmd/cmdschematicpolygonadd.h \
    schematics/cmd/cmdschematicpolygonremove.h \
    schematics/cmd/cmdschematicremove.h \
    schematics/cmd/cmdschematictextadd.h \
    schematics/cmd/cmdschematictextremove.h \
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
    schematics/items/si_netsegment.h \
    schematics/items/si_polygon.h \
    schematics/items/si_symbol.h \
    schematics/items/si_symbolpin.h \
    schematics/items/si_text.h \
    schematics/schematic.h \
    schematics/schematiclayerprovider.h \
    schematics/schematicselectionquery.h \
    settings/cmd/cmdprojectsettingschange.h \
    settings/projectsettings.h \

FORMS += \

# polyclipping
contains(UNBUNDLE, polyclipping) {
    PKGCONFIG += polyclipping
} else {
    INCLUDEPATH += ../../polyclipping
}
