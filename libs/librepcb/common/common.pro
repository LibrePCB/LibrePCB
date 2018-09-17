#-------------------------------------------------
#
# Project created by QtCreator 2015-05-31T12:55:49
#
#-------------------------------------------------

TEMPLATE = lib
TARGET = librepcbcommon

# Use common project definitions
include(../../../common.pri)

# Set preprocessor defines
DEFINES += BUILD_OUTPUT_DIRECTORY="\\\"$${OUTPUT_DIR_ABS}\\\""
DEFINES += SHARE_DIRECTORY_SOURCE="\\\"$${SHARE_DIR_ABS}\\\""
DEFINES += APP_VERSION="\\\"0.1.0-unstable\\\""
DEFINES += FILE_FORMAT_VERSION="\\\"0.1\\\""
DEFINES += GIT_COMMIT_SHA="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" rev-parse --verify HEAD)\\\""

QT += core widgets xml opengl network sql

CONFIG += staticlib

INCLUDEPATH += \
    ../../ \
    ../../fontobene \
    ../../quazip \
    ../../sexpresso \
    ../../type_safe/include \
    ../../type_safe/external/debug_assert \

SOURCES += \
    alignment.cpp \
    application.cpp \
    attributes/attribute.cpp \
    attributes/attributeprovider.cpp \
    attributes/attributesubstitutor.cpp \
    attributes/attributetype.cpp \
    attributes/attributeunit.cpp \
    attributes/attrtypecapacitance.cpp \
    attributes/attrtypefrequency.cpp \
    attributes/attrtypeinductance.cpp \
    attributes/attrtyperesistance.cpp \
    attributes/attrtypestring.cpp \
    attributes/attrtypevoltage.cpp \
    boarddesignrules.cpp \
    cam/excellongenerator.cpp \
    cam/gerberaperturelist.cpp \
    cam/gerbergenerator.cpp \
    debug.cpp \
    dialogs/aboutdialog.cpp \
    dialogs/boarddesignrulesdialog.cpp \
    dialogs/circlepropertiesdialog.cpp \
    dialogs/gridsettingsdialog.cpp \
    dialogs/holepropertiesdialog.cpp \
    dialogs/polygonpropertiesdialog.cpp \
    dialogs/stroketextpropertiesdialog.cpp \
    dialogs/textpropertiesdialog.cpp \
    exceptions.cpp \
    fileio/directorylock.cpp \
    fileio/filepath.cpp \
    fileio/fileutils.cpp \
    fileio/sexpression.cpp \
    fileio/smartfile.cpp \
    fileio/smartsexprfile.cpp \
    fileio/smarttextfile.cpp \
    fileio/smartversionfile.cpp \
    font/strokefont.cpp \
    font/strokefontpool.cpp \
    geometry/circle.cpp \
    geometry/cmd/cmdcircleedit.cpp \
    geometry/cmd/cmdholeedit.cpp \
    geometry/cmd/cmdpolygonedit.cpp \
    geometry/cmd/cmdstroketextedit.cpp \
    geometry/cmd/cmdtextedit.cpp \
    geometry/hole.cpp \
    geometry/path.cpp \
    geometry/polygon.cpp \
    geometry/stroketext.cpp \
    geometry/text.cpp \
    geometry/vertex.cpp \
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
    network/filedownload.cpp \
    network/networkaccessmanager.cpp \
    network/networkrequest.cpp \
    network/networkrequestbase.cpp \
    network/repository.cpp \
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
    utils/toolbarproxy.cpp \
    utils/undostackactiongroup.cpp \
    uuid.cpp \
    version.cpp \
    widgets/alignmentselector.cpp \
    widgets/attributelisteditorwidget.cpp \
    widgets/attributetypecombobox.cpp \
    widgets/attributeunitcombobox.cpp \
    widgets/centeredcheckbox.cpp \
    widgets/graphicslayercombobox.cpp \
    widgets/patheditorwidget.cpp \
    widgets/signalrolecombobox.cpp \
    widgets/statusbar.cpp \

HEADERS += \
    alignment.h \
    application.h \
    attributes/attribute.h \
    attributes/attributekey.h \
    attributes/attributeprovider.h \
    attributes/attributesubstitutor.h \
    attributes/attributetype.h \
    attributes/attributeunit.h \
    attributes/attrtypecapacitance.h \
    attributes/attrtypefrequency.h \
    attributes/attrtypeinductance.h \
    attributes/attrtyperesistance.h \
    attributes/attrtypestring.h \
    attributes/attrtypevoltage.h \
    boarddesignrules.h \
    cam/excellongenerator.h \
    cam/gerberaperturelist.h \
    cam/gerbergenerator.h \
    circuitidentifier.h \
    debug.h \
    dialogs/aboutdialog.h \
    dialogs/boarddesignrulesdialog.h \
    dialogs/circlepropertiesdialog.h \
    dialogs/gridsettingsdialog.h \
    dialogs/holepropertiesdialog.h \
    dialogs/polygonpropertiesdialog.h \
    dialogs/stroketextpropertiesdialog.h \
    dialogs/textpropertiesdialog.h \
    elementname.h \
    exceptions.h \
    fileio/cmd/cmdlistelementinsert.h \
    fileio/cmd/cmdlistelementremove.h \
    fileio/cmd/cmdlistelementsswap.h \
    fileio/directorylock.h \
    fileio/filepath.h \
    fileio/fileutils.h \
    fileio/serializablekeyvaluemap.h \
    fileio/serializableobject.h \
    fileio/serializableobjectlist.h \
    fileio/sexpression.h \
    fileio/smartfile.h \
    fileio/smartsexprfile.h \
    fileio/smarttextfile.h \
    fileio/smartversionfile.h \
    font/strokefont.h \
    font/strokefontpool.h \
    geometry/circle.h \
    geometry/cmd/cmdcircleedit.h \
    geometry/cmd/cmdholeedit.h \
    geometry/cmd/cmdpolygonedit.h \
    geometry/cmd/cmdstroketextedit.h \
    geometry/cmd/cmdtextedit.h \
    geometry/hole.h \
    geometry/path.h \
    geometry/polygon.h \
    geometry/stroketext.h \
    geometry/text.h \
    geometry/vertex.h \
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
    network/filedownload.h \
    network/networkaccessmanager.h \
    network/networkrequest.h \
    network/networkrequestbase.h \
    network/repository.h \
    scopeguard.h \
    scopeguardlist.h \
    signalrole.h \
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
    utils/toolbarproxy.h \
    utils/undostackactiongroup.h \
    uuid.h \
    version.h \
    widgets/alignmentselector.h \
    widgets/attributelisteditorwidget.h \
    widgets/attributetypecombobox.h \
    widgets/attributeunitcombobox.h \
    widgets/centeredcheckbox.h \
    widgets/graphicslayercombobox.h \
    widgets/patheditorwidget.h \
    widgets/signalrolecombobox.h \
    widgets/statusbar.h \

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

