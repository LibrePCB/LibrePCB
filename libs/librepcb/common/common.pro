#-------------------------------------------------
# Lib: Commonly used classes in LibrePCB
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbcommon

# Use common project definitions
include(../../../common.pri)

# Set preprocessor defines
DEFINES += BUILD_OUTPUT_DIRECTORY="\\\"$${OUTPUT_DIR_ABS}\\\""
DEFINES += SHARE_DIRECTORY_SOURCE="\\\"$${SHARE_DIR_ABS}\\\""
DEFINES += GIT_COMMIT_SHA="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" rev-parse --verify HEAD)\\\""

QT += core widgets xml opengl network sql printsupport

isEmpty(UNBUNDLE) {
    CONFIG += staticlib
} else {
    target.path = $${PREFIX}/lib
    INSTALLS += target
}

INCLUDEPATH += \
    ../../ \
    ../../sexpresso \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \
    ../../muparser/include \

RESOURCES += \
    ../../../img/images.qrc \

SOURCES += \
    algorithm/airwiresbuilder.cpp \
    alignment.cpp \
    application.cpp \
    attributes/attribute.cpp \
    attributes/attributelistmodel.cpp \
    attributes/attributeprovider.cpp \
    attributes/attributesubstitutor.cpp \
    attributes/attributetype.cpp \
    attributes/attributeunit.cpp \
    attributes/attrtypecapacitance.cpp \
    attributes/attrtypecurrent.cpp \
    attributes/attrtypefrequency.cpp \
    attributes/attrtypeinductance.cpp \
    attributes/attrtypepower.cpp \
    attributes/attrtyperesistance.cpp \
    attributes/attrtypestring.cpp \
    attributes/attrtypevoltage.cpp \
    attributes/cmd/cmdattributeedit.cpp \
    boarddesignrules.cpp \
    bom/bom.cpp \
    bom/bomcsvwriter.cpp \
    cam/excellongenerator.cpp \
    cam/gerberaperturelist.cpp \
    cam/gerbergenerator.cpp \
    debug.cpp \
    dialogs/aboutdialog.cpp \
    dialogs/boarddesignrulesdialog.cpp \
    dialogs/circlepropertiesdialog.cpp \
    dialogs/filedialog.cpp \
    dialogs/gridsettingsdialog.cpp \
    dialogs/holepropertiesdialog.cpp \
    dialogs/polygonpropertiesdialog.cpp \
    dialogs/stroketextpropertiesdialog.cpp \
    dialogs/textpropertiesdialog.cpp \
    exceptions.cpp \
    fileio/asynccopyoperation.cpp \
    fileio/csvfile.cpp \
    fileio/directorylock.cpp \
    fileio/filepath.cpp \
    fileio/fileutils.cpp \
    fileio/sexpression.cpp \
    fileio/transactionaldirectory.cpp \
    fileio/transactionalfilesystem.cpp \
    fileio/versionfile.cpp \
    font/strokefont.cpp \
    font/strokefontpool.cpp \
    geometry/circle.cpp \
    geometry/cmd/cmdcircleedit.cpp \
    geometry/cmd/cmdholeedit.cpp \
    geometry/cmd/cmdpolygonedit.cpp \
    geometry/cmd/cmdstroketextedit.cpp \
    geometry/cmd/cmdtextedit.cpp \
    geometry/hole.cpp \
    geometry/junction.cpp \
    geometry/netlabel.cpp \
    geometry/netline.cpp \
    geometry/path.cpp \
    geometry/pathmodel.cpp \
    geometry/polygon.cpp \
    geometry/stroketext.cpp \
    geometry/text.cpp \
    geometry/trace.cpp \
    geometry/vertex.cpp \
    geometry/via.cpp \
    graphics/circlegraphicsitem.cpp \
    graphics/defaultgraphicslayerprovider.cpp \
    graphics/graphicslayer.cpp \
    graphics/graphicsscene.cpp \
    graphics/graphicsview.cpp \
    graphics/holegraphicsitem.cpp \
    graphics/linegraphicsitem.cpp \
    graphics/origincrossgraphicsitem.cpp \
    graphics/polygongraphicsitem.cpp \
    graphics/primitivecirclegraphicsitem.cpp \
    graphics/primitivepathgraphicsitem.cpp \
    graphics/primitivetextgraphicsitem.cpp \
    graphics/stroketextgraphicsitem.cpp \
    graphics/textgraphicsitem.cpp \
    gridproperties.cpp \
    model/angledelegate.cpp \
    model/comboboxdelegate.cpp \
    model/lengthdelegate.cpp \
    model/sortfilterproxymodel.cpp \
    network/filedownload.cpp \
    network/networkaccessmanager.cpp \
    network/networkrequest.cpp \
    network/networkrequestbase.cpp \
    network/repository.cpp \
    pnp/pickplacecsvwriter.cpp \
    pnp/pickplacedata.cpp \
    signalrole.cpp \
    sqlitedatabase.cpp \
    systeminfo.cpp \
    toolbox.cpp \
    undocommand.cpp \
    undocommandgroup.cpp \
    undostack.cpp \
    units/angle.cpp \
    units/length.cpp \
    units/lengthunit.cpp \
    units/point.cpp \
    units/ratio.cpp \
    utils/clipperhelpers.cpp \
    utils/exclusiveactiongroup.cpp \
    utils/graphicslayerstackappearancesettings.cpp \
    utils/mathparser.cpp \
    utils/toolbarproxy.cpp \
    utils/undostackactiongroup.cpp \
    uuid.cpp \
    version.cpp \
    widgets/alignmentselector.cpp \
    widgets/angleedit.cpp \
    widgets/attributelisteditorwidget.cpp \
    widgets/attributetypecombobox.cpp \
    widgets/attributeunitcombobox.cpp \
    widgets/centeredcheckbox.cpp \
    widgets/doublespinbox.cpp \
    widgets/editabletablewidget.cpp \
    widgets/graphicslayercombobox.cpp \
    widgets/halignactiongroup.cpp \
    widgets/lengthedit.cpp \
    widgets/lengtheditbase.cpp \
    widgets/numbereditbase.cpp \
    widgets/patheditorwidget.cpp \
    widgets/plaintextedit.cpp \
    widgets/positivelengthedit.cpp \
    widgets/ratioedit.cpp \
    widgets/signalrolecombobox.cpp \
    widgets/statusbar.cpp \
    widgets/tabwidget.cpp \
    widgets/unsignedlengthedit.cpp \
    widgets/unsignedratioedit.cpp \
    widgets/valignactiongroup.cpp \

HEADERS += \
    algorithm/airwiresbuilder.h \
    alignment.h \
    application.h \
    attributes/attribute.h \
    attributes/attributekey.h \
    attributes/attributelistmodel.h \
    attributes/attributeprovider.h \
    attributes/attributesubstitutor.h \
    attributes/attributetype.h \
    attributes/attributeunit.h \
    attributes/attrtypecapacitance.h \
    attributes/attrtypecurrent.h \
    attributes/attrtypefrequency.h \
    attributes/attrtypeinductance.h \
    attributes/attrtypepower.h \
    attributes/attrtyperesistance.h \
    attributes/attrtypestring.h \
    attributes/attrtypevoltage.h \
    attributes/cmd/cmdattributeedit.h \
    boarddesignrules.h \
    bom/bom.h \
    bom/bomcsvwriter.h \
    cam/excellongenerator.h \
    cam/gerberaperturelist.h \
    cam/gerbergenerator.h \
    circuitidentifier.h \
    debug.h \
    dialogs/aboutdialog.h \
    dialogs/boarddesignrulesdialog.h \
    dialogs/circlepropertiesdialog.h \
    dialogs/filedialog.h \
    dialogs/gridsettingsdialog.h \
    dialogs/holepropertiesdialog.h \
    dialogs/polygonpropertiesdialog.h \
    dialogs/stroketextpropertiesdialog.h \
    dialogs/textpropertiesdialog.h \
    elementname.h \
    exceptions.h \
    fileio/asynccopyoperation.h \
    fileio/cmd/cmdlistelementinsert.h \
    fileio/cmd/cmdlistelementremove.h \
    fileio/cmd/cmdlistelementsswap.h \
    fileio/csvfile.h \
    fileio/directorylock.h \
    fileio/filepath.h \
    fileio/filesystem.h \
    fileio/fileutils.h \
    fileio/serializablekeyvaluemap.h \
    fileio/serializableobject.h \
    fileio/serializableobjectlist.h \
    fileio/sexpression.h \
    fileio/transactionaldirectory.h \
    fileio/transactionalfilesystem.h \
    fileio/versionfile.h \
    font/strokefont.h \
    font/strokefontpool.h \
    geometry/circle.h \
    geometry/cmd/cmdcircleedit.h \
    geometry/cmd/cmdholeedit.h \
    geometry/cmd/cmdpolygonedit.h \
    geometry/cmd/cmdstroketextedit.h \
    geometry/cmd/cmdtextedit.h \
    geometry/hole.h \
    geometry/junction.h \
    geometry/netlabel.h \
    geometry/netline.h \
    geometry/path.h \
    geometry/pathmodel.h \
    geometry/polygon.h \
    geometry/stroketext.h \
    geometry/text.h \
    geometry/trace.h \
    geometry/vertex.h \
    geometry/via.h \
    graphics/circlegraphicsitem.h \
    graphics/defaultgraphicslayerprovider.h \
    graphics/graphicslayer.h \
    graphics/graphicslayername.h \
    graphics/graphicsscene.h \
    graphics/graphicsview.h \
    graphics/holegraphicsitem.h \
    graphics/if_graphicsvieweventhandler.h \
    graphics/linegraphicsitem.h \
    graphics/origincrossgraphicsitem.h \
    graphics/polygongraphicsitem.h \
    graphics/primitivecirclegraphicsitem.h \
    graphics/primitivepathgraphicsitem.h \
    graphics/primitivetextgraphicsitem.h \
    graphics/stroketextgraphicsitem.h \
    graphics/textgraphicsitem.h \
    gridproperties.h \
    model/angledelegate.h \
    model/comboboxdelegate.h \
    model/editablelistmodel.h \
    model/lengthdelegate.h \
    model/sortfilterproxymodel.h \
    network/filedownload.h \
    network/networkaccessmanager.h \
    network/networkrequest.h \
    network/networkrequestbase.h \
    network/repository.h \
    norms.h \
    pnp/pickplacecsvwriter.h \
    pnp/pickplacedata.h \
    scopeguard.h \
    scopeguardlist.h \
    signalrole.h \
    signalslot.h \
    sqlitedatabase.h \
    systeminfo.h \
    toolbox.h \
    undocommand.h \
    undocommandgroup.h \
    undostack.h \
    units/all_length_units.h \
    units/angle.h \
    units/length.h \
    units/lengthunit.h \
    units/point.h \
    units/ratio.h \
    utils/clipperhelpers.h \
    utils/exclusiveactiongroup.h \
    utils/graphicslayerstackappearancesettings.h \
    utils/mathparser.h \
    utils/toolbarproxy.h \
    utils/undostackactiongroup.h \
    uuid.h \
    version.h \
    widgets/alignmentselector.h \
    widgets/angleedit.h \
    widgets/attributelisteditorwidget.h \
    widgets/attributetypecombobox.h \
    widgets/attributeunitcombobox.h \
    widgets/centeredcheckbox.h \
    widgets/doublespinbox.h \
    widgets/editabletablewidget.h \
    widgets/graphicslayercombobox.h \
    widgets/halignactiongroup.h \
    widgets/lengthedit.h \
    widgets/lengtheditbase.h \
    widgets/numbereditbase.h \
    widgets/patheditorwidget.h \
    widgets/plaintextedit.h \
    widgets/positivelengthedit.h \
    widgets/ratioedit.h \
    widgets/signalrolecombobox.h \
    widgets/statusbar.h \
    widgets/tabwidget.h \
    widgets/unsignedlengthedit.h \
    widgets/unsignedratioedit.h \
    widgets/valignactiongroup.h \

FORMS += \
    dialogs/aboutdialog.ui \
    dialogs/boarddesignrulesdialog.ui \
    dialogs/circlepropertiesdialog.ui \
    dialogs/gridsettingsdialog.ui \
    dialogs/holepropertiesdialog.ui \
    dialogs/polygonpropertiesdialog.ui \
    dialogs/stroketextpropertiesdialog.ui \
    dialogs/textpropertiesdialog.ui \
    widgets/alignmentselector.ui \

LIBS += -L../../../output -lmuparser -lsexpresso

# quazip
contains(UNBUNDLE, quazip) {
    PKGCONFIG += quazip
} else {
    INCLUDEPATH += ../../quazip
    LIBS += -lquazip
}

# polyclipping
contains(UNBUNDLE, polyclipping) {
    PKGCONFIG += polyclipping
} else {
    INCLUDEPATH += ../../polyclipping
    LIBS += -lpolyclipping
}

# fontobene-qt5
contains(UNBUNDLE, fontobene-qt5) {
    PKGCONFIG += fontobene-qt5
} else {
    INCLUDEPATH += ../../fontobene-qt5
}

# linking information defines (used by `Application` class)
isEmpty(UNBUNDLE) {
    DEFINES += "LINKING_TYPE=\"\\\"static\\\"\""
    DEFINES += "UNBUNDLE=\"\\\"-\\\"\""
} else {
    DEFINES += "LINKING_TYPE=\"\\\"dynamic\\\"\""
    DEFINES += "UNBUNDLE=\"\\\"$${UNBUNDLE}\\\"\""
}
