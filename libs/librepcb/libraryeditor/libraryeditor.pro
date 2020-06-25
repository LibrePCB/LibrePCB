#-------------------------------------------------
# Lib: Classes & GUI elements to modify LibrePCB library elements
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcblibraryeditor

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
    cmp/cmpsigpindisplaytypecombobox.cpp \
    cmp/componenteditorwidget.cpp \
    cmp/componentpinsignalmapmodel.cpp \
    cmp/componentsignallisteditorwidget.cpp \
    cmp/componentsignallistmodel.cpp \
    cmp/componentsymbolvarianteditdialog.cpp \
    cmp/componentsymbolvariantitemlisteditorwidget.cpp \
    cmp/componentsymbolvariantitemlistmodel.cpp \
    cmp/componentsymbolvariantlistmodel.cpp \
    cmp/componentsymbolvariantlistwidget.cpp \
    cmp/compsymbvarpinsignalmapeditorwidget.cpp \
    cmpcat/componentcategoryeditorwidget.cpp \
    common/categorychooserdialog.cpp \
    common/categorylisteditorwidget.cpp \
    common/categorytreelabeltextbuilder.cpp \
    common/componentchooserdialog.cpp \
    common/editorwidgetbase.cpp \
    common/libraryelementchecklistwidget.cpp \
    common/packagechooserdialog.cpp \
    common/symbolchooserdialog.cpp \
    dev/deviceeditorwidget.cpp \
    dev/devicepadsignalmapmodel.cpp \
    dev/padsignalmapeditorwidget.cpp \
    lib/librarylisteditorwidget.cpp \
    lib/libraryoverviewwidget.cpp \
    libraryeditor.cpp \
    libraryelementcache.cpp \
    newelementwizard/newelementwizard.cpp \
    newelementwizard/newelementwizardcontext.cpp \
    newelementwizard/newelementwizardpage_choosetype.cpp \
    newelementwizard/newelementwizardpage_componentpinsignalmap.cpp \
    newelementwizard/newelementwizardpage_componentproperties.cpp \
    newelementwizard/newelementwizardpage_componentsignals.cpp \
    newelementwizard/newelementwizardpage_componentsymbols.cpp \
    newelementwizard/newelementwizardpage_copyfrom.cpp \
    newelementwizard/newelementwizardpage_deviceproperties.cpp \
    newelementwizard/newelementwizardpage_entermetadata.cpp \
    newelementwizard/newelementwizardpage_packagepads.cpp \
    pkg/dialogs/footprintpadpropertiesdialog.cpp \
    pkg/footprintclipboarddata.cpp \
    pkg/footprintlisteditorwidget.cpp \
    pkg/footprintlistmodel.cpp \
    pkg/fsm/cmd/cmddragselectedfootprintitems.cpp \
    pkg/fsm/cmd/cmdpastefootprintitems.cpp \
    pkg/fsm/cmd/cmdremoveselectedfootprintitems.cpp \
    pkg/fsm/packageeditorfsm.cpp \
    pkg/fsm/packageeditorstate.cpp \
    pkg/fsm/packageeditorstate_addholes.cpp \
    pkg/fsm/packageeditorstate_addnames.cpp \
    pkg/fsm/packageeditorstate_addpads.cpp \
    pkg/fsm/packageeditorstate_addvalues.cpp \
    pkg/fsm/packageeditorstate_drawcircle.cpp \
    pkg/fsm/packageeditorstate_drawline.cpp \
    pkg/fsm/packageeditorstate_drawpolygon.cpp \
    pkg/fsm/packageeditorstate_drawpolygonbase.cpp \
    pkg/fsm/packageeditorstate_drawrect.cpp \
    pkg/fsm/packageeditorstate_drawtext.cpp \
    pkg/fsm/packageeditorstate_drawtextbase.cpp \
    pkg/fsm/packageeditorstate_select.cpp \
    pkg/packageeditorwidget.cpp \
    pkg/packagepadlisteditorwidget.cpp \
    pkg/packagepadlistmodel.cpp \
    pkg/widgets/boardsideselectorwidget.cpp \
    pkg/widgets/footprintpadshapeselectorwidget.cpp \
    pkg/widgets/packagepadcombobox.cpp \
    pkgcat/packagecategoryeditorwidget.cpp \
    sym/dialogs/symbolpinpropertiesdialog.cpp \
    sym/fsm/cmd/cmddragselectedsymbolitems.cpp \
    sym/fsm/cmd/cmdpastesymbolitems.cpp \
    sym/fsm/cmd/cmdremoveselectedsymbolitems.cpp \
    sym/fsm/symboleditorfsm.cpp \
    sym/fsm/symboleditorstate.cpp \
    sym/fsm/symboleditorstate_addnames.cpp \
    sym/fsm/symboleditorstate_addpins.cpp \
    sym/fsm/symboleditorstate_addvalues.cpp \
    sym/fsm/symboleditorstate_drawcircle.cpp \
    sym/fsm/symboleditorstate_drawline.cpp \
    sym/fsm/symboleditorstate_drawpolygon.cpp \
    sym/fsm/symboleditorstate_drawpolygonbase.cpp \
    sym/fsm/symboleditorstate_drawrect.cpp \
    sym/fsm/symboleditorstate_drawtext.cpp \
    sym/fsm/symboleditorstate_drawtextbase.cpp \
    sym/fsm/symboleditorstate_select.cpp \
    sym/symbolclipboarddata.cpp \
    sym/symboleditorwidget.cpp \

HEADERS += \
    cmp/cmpsigpindisplaytypecombobox.h \
    cmp/componenteditorwidget.h \
    cmp/componentpinsignalmapmodel.h \
    cmp/componentsignallisteditorwidget.h \
    cmp/componentsignallistmodel.h \
    cmp/componentsymbolvarianteditdialog.h \
    cmp/componentsymbolvariantitemlisteditorwidget.h \
    cmp/componentsymbolvariantitemlistmodel.h \
    cmp/componentsymbolvariantlistmodel.h \
    cmp/componentsymbolvariantlistwidget.h \
    cmp/compsymbvarpinsignalmapeditorwidget.h \
    cmp/if_componentsymbolvarianteditorprovider.h \
    cmpcat/componentcategoryeditorwidget.h \
    common/categorychooserdialog.h \
    common/categorylisteditorwidget.h \
    common/categorytreelabeltextbuilder.h \
    common/componentchooserdialog.h \
    common/editorwidgetbase.h \
    common/libraryelementchecklistwidget.h \
    common/packagechooserdialog.h \
    common/symbolchooserdialog.h \
    dev/deviceeditorwidget.h \
    dev/devicepadsignalmapmodel.h \
    dev/padsignalmapeditorwidget.h \
    lib/librarylisteditorwidget.h \
    lib/libraryoverviewwidget.h \
    libraryeditor.h \
    libraryelementcache.h \
    newelementwizard/newelementwizard.h \
    newelementwizard/newelementwizardcontext.h \
    newelementwizard/newelementwizardpage_choosetype.h \
    newelementwizard/newelementwizardpage_componentpinsignalmap.h \
    newelementwizard/newelementwizardpage_componentproperties.h \
    newelementwizard/newelementwizardpage_componentsignals.h \
    newelementwizard/newelementwizardpage_componentsymbols.h \
    newelementwizard/newelementwizardpage_copyfrom.h \
    newelementwizard/newelementwizardpage_deviceproperties.h \
    newelementwizard/newelementwizardpage_entermetadata.h \
    newelementwizard/newelementwizardpage_packagepads.h \
    pkg/dialogs/footprintpadpropertiesdialog.h \
    pkg/footprintclipboarddata.h \
    pkg/footprintlisteditorwidget.h \
    pkg/footprintlistmodel.h \
    pkg/fsm/cmd/cmddragselectedfootprintitems.h \
    pkg/fsm/cmd/cmdpastefootprintitems.h \
    pkg/fsm/cmd/cmdremoveselectedfootprintitems.h \
    pkg/fsm/packageeditorfsm.h \
    pkg/fsm/packageeditorstate.h \
    pkg/fsm/packageeditorstate_addholes.h \
    pkg/fsm/packageeditorstate_addnames.h \
    pkg/fsm/packageeditorstate_addpads.h \
    pkg/fsm/packageeditorstate_addvalues.h \
    pkg/fsm/packageeditorstate_drawcircle.h \
    pkg/fsm/packageeditorstate_drawline.h \
    pkg/fsm/packageeditorstate_drawpolygon.h \
    pkg/fsm/packageeditorstate_drawpolygonbase.h \
    pkg/fsm/packageeditorstate_drawrect.h \
    pkg/fsm/packageeditorstate_drawtext.h \
    pkg/fsm/packageeditorstate_drawtextbase.h \
    pkg/fsm/packageeditorstate_select.h \
    pkg/packageeditorwidget.h \
    pkg/packagepadlisteditorwidget.h \
    pkg/packagepadlistmodel.h \
    pkg/widgets/boardsideselectorwidget.h \
    pkg/widgets/footprintpadshapeselectorwidget.h \
    pkg/widgets/packagepadcombobox.h \
    pkgcat/packagecategoryeditorwidget.h \
    sym/dialogs/symbolpinpropertiesdialog.h \
    sym/fsm/cmd/cmddragselectedsymbolitems.h \
    sym/fsm/cmd/cmdpastesymbolitems.h \
    sym/fsm/cmd/cmdremoveselectedsymbolitems.h \
    sym/fsm/symboleditorfsm.h \
    sym/fsm/symboleditorstate.h \
    sym/fsm/symboleditorstate_addnames.h \
    sym/fsm/symboleditorstate_addpins.h \
    sym/fsm/symboleditorstate_addvalues.h \
    sym/fsm/symboleditorstate_drawcircle.h \
    sym/fsm/symboleditorstate_drawline.h \
    sym/fsm/symboleditorstate_drawpolygon.h \
    sym/fsm/symboleditorstate_drawpolygonbase.h \
    sym/fsm/symboleditorstate_drawrect.h \
    sym/fsm/symboleditorstate_drawtext.h \
    sym/fsm/symboleditorstate_drawtextbase.h \
    sym/fsm/symboleditorstate_select.h \
    sym/symbolclipboarddata.h \
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
    newelementwizard/newelementwizard.ui \
    newelementwizard/newelementwizardpage_choosetype.ui \
    newelementwizard/newelementwizardpage_componentpinsignalmap.ui \
    newelementwizard/newelementwizardpage_componentproperties.ui \
    newelementwizard/newelementwizardpage_componentsignals.ui \
    newelementwizard/newelementwizardpage_componentsymbols.ui \
    newelementwizard/newelementwizardpage_copyfrom.ui \
    newelementwizard/newelementwizardpage_deviceproperties.ui \
    newelementwizard/newelementwizardpage_entermetadata.ui \
    newelementwizard/newelementwizardpage_packagepads.ui \
    pkg/dialogs/footprintpadpropertiesdialog.ui \
    pkg/packageeditorwidget.ui \
    pkgcat/packagecategoryeditorwidget.ui \
    sym/dialogs/symbolpinpropertiesdialog.ui \
    sym/symboleditorwidget.ui \

