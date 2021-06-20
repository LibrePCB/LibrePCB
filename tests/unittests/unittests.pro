#-------------------------------------------------
# App: LibrePCB unit tests
#-------------------------------------------------

TEMPLATE = app
TARGET = librepcb-unittests

# Use common project definitions
include(../../common.pri)

# Set preprocessor defines
DEFINES += TEST_DATA_DIR=\\\"$${PWD}/../data\\\"

QT += core widgets network printsupport xml opengl sql concurrent testlib

CONFIG += console
CONFIG -= app_bundle

LIBS += \
    -L$${DESTDIR} \
    -lgoogletest \
    -llibrepcblibrarymanager \
    -llibrepcblibraryeditor \
    -llibrepcbprojecteditor \
    -llibrepcbeagleimport \
    -llibrepcbworkspace \
    -llibrepcbproject \
    -llibrepcblibrary \
    -llibrepcbcommon \
    -lmuparser \
    -lparseagle \

# Solaris based systems need to link against libproc
solaris:LIBS += -lproc

INCLUDEPATH += \
    ../../libs \
    ../../libs/googletest/googletest/include \
    ../../libs/googletest/googlemock/include \
    ../../libs/parseagle \
    ../../libs/type_safe/include \
    ../../libs/type_safe/external/debug_assert \

DEPENDPATH += \
    ../../libs/librepcb/librarymanager \
    ../../libs/librepcb/projecteditor \
    ../../libs/librepcb/libraryeditor \
    ../../libs/librepcb/eagleimport \
    ../../libs/librepcb/workspace \
    ../../libs/librepcb/project \
    ../../libs/librepcb/library \
    ../../libs/librepcb/common \
    ../../libs/parseagle \
    ../../libs/muparser \

PRE_TARGETDEPS += \
    $${DESTDIR}/libgoogletest.a \
    $${DESTDIR}/libmuparser.a \

isEmpty(UNBUNDLE) {
    # These libraries will only be linked statically when not unbundling
    PRE_TARGETDEPS += \
        $${DESTDIR}/liblibrepcblibrarymanager.a \
        $${DESTDIR}/liblibrepcbprojecteditor.a \
        $${DESTDIR}/liblibrepcblibraryeditor.a \
        $${DESTDIR}/liblibrepcbworkspace.a \
        $${DESTDIR}/liblibrepcbproject.a \
        $${DESTDIR}/liblibrepcblibrary.a \
        $${DESTDIR}/liblibrepcbcommon.a \
        $${DESTDIR}/libquazip.a \
        $${DESTDIR}/libpolyclipping.a \
}

SOURCES += \
    common/algorithm/airwiresbuildertest.cpp \
    common/alignmenttest.cpp \
    common/applicationtest.cpp \
    common/attributes/attributekeytest.cpp \
    common/attributes/attributesubstitutortest.cpp \
    common/attributes/attributetest.cpp \
    common/attributes/attributetypetest.cpp \
    common/attributes/attributeunittest.cpp \
    common/boarddesignrulestest.cpp \
    common/cam/gerberaperturelisttest.cpp \
    common/cam/gerberattributetest.cpp \
    common/cam/gerberattributewritertest.cpp \
    common/cam/gerbergeneratortest.cpp \
    common/circuitidentifiertest.cpp \
    common/fileio/asynccopyoperationtest.cpp \
    common/fileio/csvfiletest.cpp \
    common/fileio/directorylocktest.cpp \
    common/fileio/filepathtest.cpp \
    common/fileio/serializableobjectlisttest.cpp \
    common/fileio/sexpressiontest.cpp \
    common/fileio/transactionaldirectorytest.cpp \
    common/fileio/transactionalfilesystemtest.cpp \
    common/geometry/pathmodeltest.cpp \
    common/geometry/pathtest.cpp \
    common/geometry/polygontest.cpp \
    common/geometry/stroketexttest.cpp \
    common/geometry/texttest.cpp \
    common/geometry/tracetest.cpp \
    common/geometry/vertextest.cpp \
    common/geometry/viatest.cpp \
    common/graphics/graphicslayernametest.cpp \
    common/network/filedownloadtest.cpp \
    common/network/networkrequesttest.cpp \
    common/pnp/pickplacecsvwritertest.cpp \
    common/scopeguardtest.cpp \
    common/signalroletest.cpp \
    common/signalslottest.cpp \
    common/sqlitedatabasetest.cpp \
    common/systeminfotest.cpp \
    common/toolboxtest.cpp \
    common/units/angletest.cpp \
    common/units/lengthsnaptest.cpp \
    common/units/lengthtest.cpp \
    common/units/pointtest.cpp \
    common/units/ratiotest.cpp \
    common/utils/mathparsertest.cpp \
    common/uuidtest.cpp \
    common/versiontest.cpp \
    common/widgets/editabletablewidgettest.cpp \
    common/widgets/lengthedittest.cpp \
    common/widgets/positivelengthedittest.cpp \
    common/widgets/unsignedlengthedittest.cpp \
    eagleimport/deviceconvertertest.cpp \
    eagleimport/devicesetconvertertest.cpp \
    eagleimport/packageconvertertest.cpp \
    eagleimport/symbolconvertertest.cpp \
    library/cmp/componentprefixtest.cpp \
    library/cmp/componentsymbolvariantitemsuffixtest.cpp \
    library/cmp/componentsymbolvariantitemtest.cpp \
    library/librarybaseelementtest.cpp \
    library/pkg/footprintpadtest.cpp \
    library/sym/symbolpintest.cpp \
    libraryeditor/pkg/footprintclipboarddatatest.cpp \
    libraryeditor/sym/symbolclipboarddatatest.cpp \
    librarymanager/librarydownloadtest.cpp \
    main.cpp \
    project/boards/boardfabricationoutputsettingstest.cpp \
    project/boards/boardgerberexporttest.cpp \
    project/boards/boardpickplacegeneratortest.cpp \
    project/boards/boardplanefragmentsbuildertest.cpp \
    project/library/projectlibrarytest.cpp \
    project/projecttest.cpp \
    projecteditor/boardeditor/boardclipboarddatatest.cpp \
    projecteditor/schematiceditor/schematicclipboarddatatest.cpp \
    workspace/settings/workspacesettingstest.cpp \
    workspace/workspacetest.cpp \

HEADERS += \
    common/attributes/attributeproviderdummy.h \
    common/fileio/serializableobjectmock.h \
    common/network/networkrequestbasesignalreceiver.h \
    common/widgets/editabletablewidgetreceiver.h \

FORMS += \

# Hoedown and markdownconverter are only needed for Qt <5.14
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 14) {
    LIBS += -lhoedown
    DEPENDPATH += ../../libs/hoedown
    PRE_TARGETDEPS += $${DESTDIR}/libhoedown.a
}

# QuaZIP
!contains(UNBUNDLE, quazip) {
    LIBS += -lquazip -lz
    INCLUDEPATH += ../../libs/quazip
    DEPENDPATH += ../../libs/quazip
}

# polyclipping
!contains(UNBUNDLE, polyclipping) {
    LIBS += -lpolyclipping
    DEPENDPATH += ../../libs/polyclipping
}
