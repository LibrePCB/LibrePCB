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
  common/algorithm/airwiresbuildertest.cpp
  common/alignmenttest.cpp
  common/applicationtest.cpp
  common/attributes/attributekeytest.cpp
  common/attributes/attributeproviderdummy.h
  common/attributes/attributesubstitutortest.cpp
  common/attributes/attributetest.cpp
  common/attributes/attributetypetest.cpp
  common/attributes/attributeunittest.cpp
  common/boarddesignrulestest.cpp
  common/cam/gerberaperturelisttest.cpp
  common/cam/gerberattributetest.cpp
  common/cam/gerberattributewritertest.cpp
  common/cam/gerbergeneratortest.cpp
  common/circuitidentifiertest.cpp
  common/fileio/asynccopyoperationtest.cpp
  common/fileio/csvfiletest.cpp
  common/fileio/directorylocktest.cpp
  common/fileio/filepathtest.cpp
  common/fileio/serializableobjectlisttest.cpp
  common/fileio/serializableobjectmock.h
  common/fileio/sexpressiontest.cpp
  common/fileio/transactionaldirectorytest.cpp
  common/fileio/transactionalfilesystemtest.cpp
  common/geometry/pathmodeltest.cpp
  common/geometry/pathtest.cpp
  common/geometry/polygontest.cpp
  common/geometry/stroketexttest.cpp
  common/geometry/texttest.cpp
  common/geometry/tracetest.cpp
  common/geometry/vertextest.cpp
  common/geometry/viatest.cpp
  common/graphics/graphicslayernametest.cpp
  common/network/filedownloadtest.cpp
  common/network/networkrequestbasesignalreceiver.h
  common/network/networkrequesttest.cpp
  common/pnp/pickplacecsvwritertest.cpp
  common/scopeguardtest.cpp
  common/signalroletest.cpp
  common/signalslottest.cpp
  common/sqlitedatabasetest.cpp
  common/systeminfotest.cpp
  common/toolboxtest.cpp
  common/units/angletest.cpp
  common/units/lengthsnaptest.cpp
  common/units/lengthtest.cpp
  common/units/pointtest.cpp
  common/units/ratiotest.cpp
  common/utils/mathparsertest.cpp
  common/uuidtest.cpp
  common/versiontest.cpp
  common/widgets/editabletablewidgetreceiver.h
  common/widgets/editabletablewidgettest.cpp
  common/widgets/lengthedittest.cpp
  common/widgets/positivelengthedittest.cpp
  common/widgets/unsignedlengthedittest.cpp
  eagleimport/deviceconvertertest.cpp
  eagleimport/devicesetconvertertest.cpp
  eagleimport/packageconvertertest.cpp
  eagleimport/symbolconvertertest.cpp
  library/cmp/componentprefixtest.cpp
  library/cmp/componentsymbolvariantitemsuffixtest.cpp
  library/cmp/componentsymbolvariantitemtest.cpp
  library/librarybaseelementtest.cpp
  library/pkg/footprintpadtest.cpp
  library/sym/symbolpintest.cpp
  libraryeditor/pkg/footprintclipboarddatatest.cpp
  libraryeditor/sym/symbolclipboarddatatest.cpp
  librarymanager/librarydownloadtest.cpp
  main.cpp
  project/boards/boardfabricationoutputsettingstest.cpp
  project/boards/boardgerberexporttest.cpp
  project/boards/boardpickplacegeneratortest.cpp
  project/boards/boardplanefragmentsbuildertest.cpp
  project/library/projectlibrarytest.cpp
  project/projecttest.cpp
  projecteditor/boardeditor/boardclipboarddatatest.cpp
  projecteditor/schematiceditor/schematicclipboarddatatest.cpp
  workspace/settings/workspacesettingstest.cpp
  workspace/workspacetest.cpp
)
target_include_directories(
  librepcb_unittests
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libs"
)
target_link_libraries(
  librepcb_unittests
  PRIVATE common
          # LibrePCB
          LibrePCB::EagleImport
          LibrePCB::LibraryManager
          LibrePCB::LibraryEditor
          LibrePCB::ProjectEditor
          LibrePCB::Workspace
          LibrePCB::Project
          LibrePCB::Library
          LibrePCB::Common
          # Third party
          ParsEagle::ParsEagle
          GTest::GTest
          GTest::GMock
          # Qt
          Qt5::Concurrent
          Qt5::Test
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
