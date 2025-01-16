# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Path to test data
add_definitions(-DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data")

# Tests require libpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Main executable
add_executable(
  librepcb_unittests
  core/3d/occmodeltest.cpp
  core/algorithm/airwiresbuildertest.cpp
  core/applicationtest.cpp
  core/attribute/attributekeytest.cpp
  core/attribute/attributesubstitutortest.cpp
  core/attribute/attributetest.cpp
  core/attribute/attributetypetest.cpp
  core/attribute/attributeunittest.cpp
  core/export/d356netlistgeneratortest.cpp
  core/export/excellongeneratortest.cpp
  core/export/gerberaperturelisttest.cpp
  core/export/gerberattributetest.cpp
  core/export/gerberattributewritertest.cpp
  core/export/gerbergeneratortest.cpp
  core/export/graphicsexporttest.cpp
  core/export/graphicsexporttest.h
  core/export/interactivehtmlbomtest.cpp
  core/export/pickplacecsvwritertest.cpp
  core/fileio/asynccopyoperationtest.cpp
  core/fileio/csvfiletest.cpp
  core/fileio/directorylocktest.cpp
  core/fileio/filepathtest.cpp
  core/fileio/fileutilstest.cpp
  core/fileio/transactionaldirectorytest.cpp
  core/fileio/transactionalfilesystemtest.cpp
  core/fileio/versionfiletest.cpp
  core/geometry/holetest.cpp
  core/geometry/pathtest.cpp
  core/geometry/polygontest.cpp
  core/geometry/stroketexttest.cpp
  core/geometry/texttest.cpp
  core/geometry/tracetest.cpp
  core/geometry/vertextest.cpp
  core/geometry/viatest.cpp
  core/import/dxfreadertest.cpp
  core/library/cmp/componentprefixtest.cpp
  core/library/cmp/componentsymbolvariantitemsuffixtest.cpp
  core/library/cmp/componentsymbolvariantitemtest.cpp
  core/library/cmp/componenttest.cpp
  core/library/cmpcat/componentcategorytest.cpp
  core/library/dev/devicetest.cpp
  core/library/librarybaseelementtest.cpp
  core/library/librarytest.cpp
  core/library/pkg/footprintpadtest.cpp
  core/library/pkg/packagetest.cpp
  core/library/pkgcat/packagecategorytest.cpp
  core/library/sym/symbolpintest.cpp
  core/library/sym/symboltest.cpp
  core/network/filedownloadtest.cpp
  core/network/networkrequestbasesignalreceiver.h
  core/network/networkrequesttest.cpp
  core/project/board/boardd356netlistexporttest.cpp
  core/project/board/boarddesignrulechecktest.cpp
  core/project/board/boarddesignrulestest.cpp
  core/project/board/boardfabricationoutputsettingstest.cpp
  core/project/board/boardgerberexporttest.cpp
  core/project/board/boardinteractivehtmlbomgeneratortest.cpp
  core/project/board/boardpickplacegeneratortest.cpp
  core/project/board/boardplanefragmentsbuildertest.cpp
  core/project/board/boardspecctraexporttest.cpp
  core/project/projectjsonexporttest.cpp
  core/project/projectlibrarytest.cpp
  core/project/projecttest.cpp
  core/serialization/serializableobjectlisttest.cpp
  core/serialization/serializableobjectmock.h
  core/serialization/sexpressiontest.cpp
  core/sqlitedatabasetest.cpp
  core/systeminfotest.cpp
  core/types/alignmenttest.cpp
  core/types/angletest.cpp
  core/types/circuitidentifiertest.cpp
  core/types/fileproofnametest.cpp
  core/types/lengthtest.cpp
  core/types/pointtest.cpp
  core/types/ratiotest.cpp
  core/types/signalroletest.cpp
  core/types/simplestringtest.cpp
  core/types/uuidtest.cpp
  core/types/versiontest.cpp
  core/utils/clipperhelperstest.cpp
  core/utils/mathparsertest.cpp
  core/utils/overlinemarkupparsertest.cpp
  core/utils/scopeguardtest.cpp
  core/utils/signalslottest.cpp
  core/utils/tangentpathjoinertest.cpp
  core/utils/toolboxtest.cpp
  core/utils/transformtest.cpp
  core/workspace/workspacelibrarydbtest.cpp
  core/workspace/workspacesettingstest.cpp
  core/workspace/workspacetest.cpp
  eagleimport/eaglelibraryimporttest.cpp
  eagleimport/eagleprojectimporttest.cpp
  eagleimport/eagletypeconvertertest.cpp
  editor/dialogs/dxfimportdialogtest.cpp
  editor/dialogs/graphicsexportdialogtest.cpp
  editor/library/cat/categorytreebuildertest.cpp
  editor/library/pkg/footprintclipboarddatatest.cpp
  editor/library/sym/symbolclipboarddatatest.cpp
  editor/modelview/pathmodeltest.cpp
  editor/project/addcomponentdialogtest.cpp
  editor/project/boardeditor/boardclipboarddatatest.cpp
  editor/project/boardeditor/cmdboardspecctraimporttest.cpp
  editor/project/orderpcbdialogtest.cpp
  editor/project/schematiceditor/schematicclipboarddatatest.cpp
  editor/utils/shortcutsreferencegeneratortest.cpp
  editor/widgets/editabletablewidgetreceiver.h
  editor/widgets/editabletablewidgettest.cpp
  editor/widgets/lengthedittest.cpp
  editor/widgets/positivelengthedittest.cpp
  editor/widgets/unsignedlengthedittest.cpp
  editor/workspace/categorytreemodeltest.cpp
  editor/workspace/librarymanager/librarydownloadtest.cpp
  kicadimport/kicadlibraryimporttest.cpp
  main.cpp
  testhelpers.cpp
  testhelpers.h
)
target_include_directories(
  librepcb_unittests
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  librepcb_unittests
  PRIVATE common
          # LibrePCB
          LibrePCB::Editor
          LibrePCB::Core
          # Third party
          GTest::GTest
          GTest::GMock
          ParsEagle::ParsEagle
          # Qt
          ${QT}::Concurrent
          ${QT}::Core
          ${QT}::Gui
          ${QT}::Test
          ${QT}::Widgets
          # System
          Threads::Threads
)
set_target_properties(
  librepcb_unittests PROPERTIES OUTPUT_NAME librepcb-unittests
)

# Additional executable that's used for process tests
add_executable(
  dummy_binary
  dummy.cpp
)
set_target_properties(dummy_binary PROPERTIES OUTPUT_NAME dummy-binary)
