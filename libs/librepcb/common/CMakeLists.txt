# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Allow overriding the share directory path
set(LIBREPCB_SHARE
    "../share/librepcb"
    CACHE STRING "Path to the LibrePCB share directory."
)

# Set this to YES to disable some features that make builds non-reproducible
# (like embedding the git commit hash or the path to the source directory in
# the binary).
set(LIBREPCB_REPRODUCIBLE
    NO
    CACHE BOOL "Disable feature that make builds non-reproducible"
)

if(LIBREPCB_REPRODUCIBLE)
  message(STATUS "Creating a potentially reproducible build")
else()
  # Detect git commit hash
  find_package(Git)
  if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} -C "${CMAKE_SOURCE_DIR}" rev-parse --short
              --verify HEAD
      OUTPUT_VARIABLE SHORT_SHA
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(GIT_COMMIT_SHA ${SHORT_SHA})
  else()
    message(STATUS "Git not found, cannot set version info")
  endif()

  # Detect path to source share directory
  set(LIBREPCB_SHARE_SOURCE "${CMAKE_SOURCE_DIR}/share")

  # Detect path to binary output directory
  set(LIBREPCB_BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()

# Generate build_env.h file
configure_file(build_env.h.in build_env.h @ONLY)

# Export library
add_library(
  librepcb_common STATIC
  algorithm/airwiresbuilder.cpp
  algorithm/airwiresbuilder.h
  alignment.cpp
  alignment.h
  application.cpp
  application.h
  attributes/attribute.cpp
  attributes/attribute.h
  attributes/attributekey.h
  attributes/attributelistmodel.cpp
  attributes/attributelistmodel.h
  attributes/attributeprovider.cpp
  attributes/attributeprovider.h
  attributes/attributesubstitutor.cpp
  attributes/attributesubstitutor.h
  attributes/attributetype.cpp
  attributes/attributetype.h
  attributes/attributeunit.cpp
  attributes/attributeunit.h
  attributes/attrtypecapacitance.cpp
  attributes/attrtypecapacitance.h
  attributes/attrtypecurrent.cpp
  attributes/attrtypecurrent.h
  attributes/attrtypefrequency.cpp
  attributes/attrtypefrequency.h
  attributes/attrtypeinductance.cpp
  attributes/attrtypeinductance.h
  attributes/attrtypepower.cpp
  attributes/attrtypepower.h
  attributes/attrtyperesistance.cpp
  attributes/attrtyperesistance.h
  attributes/attrtypestring.cpp
  attributes/attrtypestring.h
  attributes/attrtypevoltage.cpp
  attributes/attrtypevoltage.h
  attributes/cmd/cmdattributeedit.cpp
  attributes/cmd/cmdattributeedit.h
  boarddesignrules.cpp
  boarddesignrules.h
  bom/bom.cpp
  bom/bom.h
  bom/bomcsvwriter.cpp
  bom/bomcsvwriter.h
  cam/excellongenerator.cpp
  cam/excellongenerator.h
  cam/gerberaperturelist.cpp
  cam/gerberaperturelist.h
  cam/gerberattribute.cpp
  cam/gerberattribute.h
  cam/gerberattributewriter.cpp
  cam/gerberattributewriter.h
  cam/gerbergenerator.cpp
  cam/gerbergenerator.h
  circuitidentifier.h
  debug.cpp
  debug.h
  dialogs/aboutdialog.cpp
  dialogs/aboutdialog.h
  dialogs/aboutdialog.ui
  dialogs/boarddesignrulesdialog.cpp
  dialogs/boarddesignrulesdialog.h
  dialogs/boarddesignrulesdialog.ui
  dialogs/circlepropertiesdialog.cpp
  dialogs/circlepropertiesdialog.h
  dialogs/circlepropertiesdialog.ui
  dialogs/directorylockhandlerdialog.cpp
  dialogs/directorylockhandlerdialog.h
  dialogs/directorylockhandlerdialog.ui
  dialogs/dxfimportdialog.cpp
  dialogs/dxfimportdialog.h
  dialogs/dxfimportdialog.ui
  dialogs/filedialog.cpp
  dialogs/filedialog.h
  dialogs/gridsettingsdialog.cpp
  dialogs/gridsettingsdialog.h
  dialogs/gridsettingsdialog.ui
  dialogs/holepropertiesdialog.cpp
  dialogs/holepropertiesdialog.h
  dialogs/holepropertiesdialog.ui
  dialogs/polygonpropertiesdialog.cpp
  dialogs/polygonpropertiesdialog.h
  dialogs/polygonpropertiesdialog.ui
  dialogs/stroketextpropertiesdialog.cpp
  dialogs/stroketextpropertiesdialog.h
  dialogs/stroketextpropertiesdialog.ui
  dialogs/textpropertiesdialog.cpp
  dialogs/textpropertiesdialog.h
  dialogs/textpropertiesdialog.ui
  elementname.h
  exceptions.cpp
  exceptions.h
  fileio/asynccopyoperation.cpp
  fileio/asynccopyoperation.h
  fileio/cmd/cmdlistelementinsert.h
  fileio/cmd/cmdlistelementremove.h
  fileio/cmd/cmdlistelementsswap.h
  fileio/csvfile.cpp
  fileio/csvfile.h
  fileio/directorylock.cpp
  fileio/directorylock.h
  fileio/filepath.cpp
  fileio/filepath.h
  fileio/filesystem.h
  fileio/fileutils.cpp
  fileio/fileutils.h
  fileio/serializablekeyvaluemap.h
  fileio/serializableobject.h
  fileio/serializableobjectlist.h
  fileio/sexpression.cpp
  fileio/sexpression.h
  fileio/transactionaldirectory.cpp
  fileio/transactionaldirectory.h
  fileio/transactionalfilesystem.cpp
  fileio/transactionalfilesystem.h
  fileio/versionfile.cpp
  fileio/versionfile.h
  font/strokefont.cpp
  font/strokefont.h
  font/strokefontpool.cpp
  font/strokefontpool.h
  geometry/circle.cpp
  geometry/circle.h
  geometry/cmd/cmdcircleedit.cpp
  geometry/cmd/cmdcircleedit.h
  geometry/cmd/cmdholeedit.cpp
  geometry/cmd/cmdholeedit.h
  geometry/cmd/cmdpolygonedit.cpp
  geometry/cmd/cmdpolygonedit.h
  geometry/cmd/cmdstroketextedit.cpp
  geometry/cmd/cmdstroketextedit.h
  geometry/cmd/cmdtextedit.cpp
  geometry/cmd/cmdtextedit.h
  geometry/hole.cpp
  geometry/hole.h
  geometry/junction.cpp
  geometry/junction.h
  geometry/netlabel.cpp
  geometry/netlabel.h
  geometry/netline.cpp
  geometry/netline.h
  geometry/path.cpp
  geometry/path.h
  geometry/pathmodel.cpp
  geometry/pathmodel.h
  geometry/polygon.cpp
  geometry/polygon.h
  geometry/stroketext.cpp
  geometry/stroketext.h
  geometry/text.cpp
  geometry/text.h
  geometry/trace.cpp
  geometry/trace.h
  geometry/vertex.cpp
  geometry/vertex.h
  geometry/via.cpp
  geometry/via.h
  graphics/circlegraphicsitem.cpp
  graphics/circlegraphicsitem.h
  graphics/defaultgraphicslayerprovider.cpp
  graphics/defaultgraphicslayerprovider.h
  graphics/graphicslayer.cpp
  graphics/graphicslayer.h
  graphics/graphicslayername.h
  graphics/graphicsscene.cpp
  graphics/graphicsscene.h
  graphics/graphicsview.cpp
  graphics/graphicsview.h
  graphics/holegraphicsitem.cpp
  graphics/holegraphicsitem.h
  graphics/if_graphicsvieweventhandler.h
  graphics/linegraphicsitem.cpp
  graphics/linegraphicsitem.h
  graphics/origincrossgraphicsitem.cpp
  graphics/origincrossgraphicsitem.h
  graphics/polygongraphicsitem.cpp
  graphics/polygongraphicsitem.h
  graphics/primitivecirclegraphicsitem.cpp
  graphics/primitivecirclegraphicsitem.h
  graphics/primitivepathgraphicsitem.cpp
  graphics/primitivepathgraphicsitem.h
  graphics/primitivetextgraphicsitem.cpp
  graphics/primitivetextgraphicsitem.h
  graphics/stroketextgraphicsitem.cpp
  graphics/stroketextgraphicsitem.h
  graphics/textgraphicsitem.cpp
  graphics/textgraphicsitem.h
  gridproperties.cpp
  gridproperties.h
  import/dxfreader.cpp
  import/dxfreader.h
  model/angledelegate.cpp
  model/angledelegate.h
  model/comboboxdelegate.cpp
  model/comboboxdelegate.h
  model/editablelistmodel.h
  model/lengthdelegate.cpp
  model/lengthdelegate.h
  model/sortfilterproxymodel.cpp
  model/sortfilterproxymodel.h
  network/filedownload.cpp
  network/filedownload.h
  network/networkaccessmanager.cpp
  network/networkaccessmanager.h
  network/networkrequest.cpp
  network/networkrequest.h
  network/networkrequestbase.cpp
  network/networkrequestbase.h
  network/orderpcbapirequest.cpp
  network/orderpcbapirequest.h
  network/repository.cpp
  network/repository.h
  norms.h
  pnp/pickplacecsvwriter.cpp
  pnp/pickplacecsvwriter.h
  pnp/pickplacedata.cpp
  pnp/pickplacedata.h
  scopeguard.h
  scopeguardlist.h
  signalrole.cpp
  signalrole.h
  signalslot.h
  sqlitedatabase.cpp
  sqlitedatabase.h
  systeminfo.cpp
  systeminfo.h
  toolbox.cpp
  toolbox.h
  undocommand.cpp
  undocommand.h
  undocommandgroup.cpp
  undocommandgroup.h
  undostack.cpp
  undostack.h
  units/all_length_units.h
  units/angle.cpp
  units/angle.h
  units/length.cpp
  units/length.h
  units/lengthunit.cpp
  units/lengthunit.h
  units/point.cpp
  units/point.h
  units/ratio.cpp
  units/ratio.h
  utils/clipperhelpers.cpp
  utils/clipperhelpers.h
  utils/exclusiveactiongroup.cpp
  utils/exclusiveactiongroup.h
  utils/graphicslayerstackappearancesettings.cpp
  utils/graphicslayerstackappearancesettings.h
  utils/mathparser.cpp
  utils/mathparser.h
  utils/tangentpathjoiner.cpp
  utils/tangentpathjoiner.h
  utils/toolbarproxy.cpp
  utils/toolbarproxy.h
  utils/undostackactiongroup.cpp
  utils/undostackactiongroup.h
  uuid.cpp
  uuid.h
  version.cpp
  version.h
  widgets/alignmentselector.cpp
  widgets/alignmentselector.h
  widgets/alignmentselector.ui
  widgets/angleedit.cpp
  widgets/angleedit.h
  widgets/attributelisteditorwidget.cpp
  widgets/attributelisteditorwidget.h
  widgets/attributetypecombobox.cpp
  widgets/attributetypecombobox.h
  widgets/attributeunitcombobox.cpp
  widgets/attributeunitcombobox.h
  widgets/centeredcheckbox.cpp
  widgets/centeredcheckbox.h
  widgets/doublespinbox.cpp
  widgets/doublespinbox.h
  widgets/editabletablewidget.cpp
  widgets/editabletablewidget.h
  widgets/graphicslayercombobox.cpp
  widgets/graphicslayercombobox.h
  widgets/halignactiongroup.cpp
  widgets/halignactiongroup.h
  widgets/lengthedit.cpp
  widgets/lengthedit.h
  widgets/lengtheditbase.cpp
  widgets/lengtheditbase.h
  widgets/numbereditbase.cpp
  widgets/numbereditbase.h
  widgets/patheditorwidget.cpp
  widgets/patheditorwidget.h
  widgets/plaintextedit.cpp
  widgets/plaintextedit.h
  widgets/positivelengthedit.cpp
  widgets/positivelengthedit.h
  widgets/ratioedit.cpp
  widgets/ratioedit.h
  widgets/signalrolecombobox.cpp
  widgets/signalrolecombobox.h
  widgets/statusbar.cpp
  widgets/statusbar.h
  widgets/tabwidget.cpp
  widgets/tabwidget.h
  widgets/unsignedlengthedit.cpp
  widgets/unsignedlengthedit.h
  widgets/unsignedratioedit.cpp
  widgets/unsignedratioedit.h
  widgets/valignactiongroup.cpp
  widgets/valignactiongroup.h
)
target_include_directories(
  librepcb_common
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_common
  PRIVATE common Dxflib::Dxflib
)
target_link_libraries(
  librepcb_common
  PUBLIC # Third party
         TypeSafe::TypeSafe
         DelaunayTriangulation::DelaunayTriangulation
         Optional::Optional
         QuaZip::QuaZip
         MuParser::MuParser
         Polyclipping::Polyclipping
         FontoBene::FontoBeneQt5
         # Qt
         Qt5::Concurrent
         Qt5::Network
         Qt5::OpenGL
         Qt5::PrintSupport
         Qt5::Sql
         Qt5::Widgets
         # Link with -lproc on SunOS / Solaris / Illumos
         # This is required for proc_get_psinfo
         $<$<STREQUAL:$<PLATFORM_ID>,SunOS>:proc>
)

# Alias to namespaced variant
add_library(LibrePCB::Common ALIAS librepcb_common)
