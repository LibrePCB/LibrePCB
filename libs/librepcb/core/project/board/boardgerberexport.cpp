/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardgerberexport.h"

#include "../../attribute/attributesubstitutor.h"
#include "../../export/excellongenerator.h"
#include "../../export/gerbergenerator.h"
#include "../../fileio/fileutils.h"
#include "../../geometry/hole.h"
#include "../../library/cmp/componentsignal.h"
#include "../../library/pkg/footprint.h"
#include "../../library/pkg/footprintpad.h"
#include "../../library/pkg/package.h"
#include "../../library/pkg/packagepad.h"
#include "../../utils/transform.h"
#include "../circuit/componentinstance.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "../projectattributelookup.h"
#include "board.h"
#include "boardfabricationoutputsettings.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

// Required for correct sorting of QMap<LayerPair, T>.
static bool operator<(const BoardGerberExport::LayerPair& lhs,
                      const BoardGerberExport::LayerPair& rhs) noexcept {
  return Layer::lessThan(lhs.first, rhs.first) ||
      ((lhs.first == rhs.first) && (Layer::lessThan(lhs.second, rhs.second)));
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardGerberExport::BoardGerberExport(const Board& board) noexcept
  : mProject(board.getProject()),
    mBoard(board),
    mRemoveObsoleteFiles(true),
    mBeforeWriteCallback(),
    mCreationDateTime(QDateTime::currentDateTime()),
    mProjectName(*mProject.getName()),
    mCurrentInnerCopperLayer(0),
    mCurrentStartLayer(nullptr),
    mCurrentEndLayer(nullptr) {
  // If the project contains multiple boards, add the board name to the
  // Gerber file metadata as well to distinguish between the different boards.
  if (mProject.getBoards().count() > 1) {
    mProjectName += " (" % mBoard.getName() % ")";
  }
}

BoardGerberExport::~BoardGerberExport() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

FilePath BoardGerberExport::getOutputDirectory(
    const BoardFabricationOutputSettings& settings) const noexcept {
  return getOutputFilePath(settings.getOutputBasePath() + "dummy")
      .getParentDir();  // use dummy suffix
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BoardGerberExport::setRemoveObsoleteFiles(bool remove) {
  mRemoveObsoleteFiles = remove;
}

void BoardGerberExport::setBeforeWriteCallback(BeforeWriteCallback cb) {
  mBeforeWriteCallback = cb;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardGerberExport::exportPcbLayers(
    const BoardFabricationOutputSettings& settings) const {
  mWrittenFiles.clear();

  exportDrillsMerged(settings);
  exportDrillsNpth(settings);
  exportDrillsPth(settings);
  exportDrillsBlindBuried(settings);
  exportLayerBoardOutlines(settings);
  exportLayerTopCopper(settings);
  exportLayerInnerCopper(settings);
  exportLayerBottomCopper(settings);
  exportLayerTopSolderMask(settings);
  exportLayerBottomSolderMask(settings);
  exportLayerTopSilkscreen(settings);
  exportLayerBottomSilkscreen(settings);
  exportLayerTopSolderPaste(settings);
  exportLayerBottomSolderPaste(settings);
}

void BoardGerberExport::exportGlueLayer(BoardSide side,
                                        const Uuid& assemblyVariant,
                                        const FilePath& filePath) const {
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      *mProject.getVersion());
  if (side == BoardSide::Top) {
    gen.setFileFunctionGlue(GerberGenerator::BoardSide::Top,
                            GerberGenerator::Polarity::Positive);
    drawGlueLayer(gen, Layer::topGlue(), assemblyVariant);
  } else {
    gen.setFileFunctionGlue(GerberGenerator::BoardSide::Bottom,
                            GerberGenerator::Polarity::Positive);
    drawGlueLayer(gen, Layer::botGlue(), assemblyVariant);
  }
  gen.generate();
  trackFileBeforeWrite(filePath);  // can throw
  gen.saveToFile(filePath);
}

void BoardGerberExport::exportComponentLayer(BoardSide side,
                                             const Uuid& assemblyVariant,
                                             const FilePath& filePath) const {
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      *mProject.getVersion());
  if (side == BoardSide::Top) {
    gen.setFileFunctionComponent(1, GerberGenerator::BoardSide::Top);
  } else {
    gen.setFileFunctionComponent(mBoard.getInnerLayerCount() + 2,
                                 GerberGenerator::BoardSide::Bottom);
  }

  // Export board outline since this is useful for manual review.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    Q_ASSERT(polygon);
    if (polygon->getData().getLayer().isBoardEdge()) {
      UnsignedLength lineWidth = calcWidthOfLayer(
          polygon->getData().getLineWidth(), polygon->getData().getLayer());
      gen.drawPathOutline(polygon->getData().getPath(), lineWidth,
                          GerberAttribute::ApertureFunction::Profile,
                          std::nullopt, QString());
    }
  }

  // Export all components on the selected board side.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    if (device->getMirrored() == (side == BoardSide::Bottom)) {
      auto part = device->getParts(assemblyVariant).value(0);
      if (!part) {
        continue;  // Do not mount.
      }

      // Determine assembly type.
      const Package::AssemblyType assemblyType =
          device->getLibPackage().getAssemblyType(true);
      GerberGenerator::MountType mountType = GerberGenerator::MountType::Other;
      switch (assemblyType) {
        case Package::AssemblyType::None:
          qWarning()
              << "Exported device with non-mountable package to Gerber X3:"
              << *device->getComponentInstance().getName();
          break;
        case Package::AssemblyType::Tht:
        case Package::AssemblyType::Mixed:  // Does this make sense?!
          mountType = GerberGenerator::MountType::Tht;
          break;
        case Package::AssemblyType::Smt:
          mountType = GerberGenerator::MountType::Smt;
          break;
        case Package::AssemblyType::Other:
          mountType = GerberGenerator::MountType::Other;
          break;
        default:
          qWarning() << "Unknown assembly type:"
                     << static_cast<int>(assemblyType);
          break;
      }

      // Export component center and attributes.
      ProjectAttributeLookup lookup(*device, part);
      const Angle rotation = device->getRotation();
      const QString designator = *device->getComponentInstance().getName();
      const QString value =
          AttributeSubstitutor::substitute(lookup("VALUE"), lookup).trimmed();
      const QString mpn = lookup("MPN").simplified();
      const QString manufacturer = lookup("MANUFACTURER").simplified();

      // Note: Always use english locale to make PnP files portable.
      const QString footprintName =
          *device->getLibPackage().getNames().getDefaultValue();
      gen.flashComponent(device->getPosition(), rotation, designator, value,
                         mountType, manufacturer, mpn, footprintName);

      // Export component body outlines.
      QVector<std::pair<GerberAttribute::ApertureFunction, Path>> outlines;
      const Layer& outlinesLayer = (side == BoardSide::Top)
          ? Layer::topPackageOutlines()
          : Layer::botPackageOutlines();
      foreach (const Path& p, getComponentOutlines(*device, outlinesLayer)) {
        outlines.append(std::make_pair(
            GerberAttribute::ApertureFunction::ComponentOutlineBody, p));
      }
      if (outlines.isEmpty()) {
        // Many packages probably don't have an explicit package outline, thus
        // using the documentation layer as a fallback.
        const Layer& documentationLayer = (side == BoardSide::Top)
            ? Layer::topDocumentation()
            : Layer::botDocumentation();
        foreach (const Path& p,
                 getComponentOutlines(*device, documentationLayer)) {
          outlines.append(std::make_pair(
              GerberAttribute::ApertureFunction::ComponentOutlineBody, p));
        }
      }
      const Layer& courtyardLayer = (side == BoardSide::Top)
          ? Layer::topCourtyard()
          : Layer::botCourtyard();
      foreach (const Path& p, getComponentOutlines(*device, courtyardLayer)) {
        outlines.append(std::make_pair(
            GerberAttribute::ApertureFunction::ComponentOutlineCourtyard, p));
      }
      for (const auto& pair : outlines) {
        gen.drawComponentOutline(pair.second, rotation, designator, value,
                                 mountType, manufacturer, mpn, footprintName,
                                 pair.first);
      }

      // Export component pins.
      foreach (const BI_FootprintPad* pad, device->getPads()) {
        if (pad->getLibPad().getFunctionIsFiducial()) {
          continue;
        }
        QString pinName, pinSignal;
        if (const PackagePad* pkgPad = pad->getLibPackagePad()) {
          pinName = *pkgPad->getName();
        }
        if (ComponentSignalInstance* cmpSig =
                pad->getComponentSignalInstance()) {
          pinSignal = *cmpSig->getCompSignal().getName();
        }
        bool isPin1 = (pinName == "1");  // Very sophisticated algorithm ;-)
        gen.flashComponentPin(pad->getPosition(), rotation, designator, value,
                              mountType, manufacturer, mpn, footprintName,
                              pinName, pinSignal, isPin1);
      }
    }
  }

  // Export fiducials on the selected board side.
  const Layer& cuLayer =
      (side == BoardSide::Bottom) ? Layer::botCopper() : Layer::topCopper();
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    int padNumber = 1;
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      if (pad->getLibPad().getFunctionIsFiducial() && pad->isOnLayer(cuLayer)) {
        ProjectAttributeLookup lookup(*device, nullptr);
        const QString designator =
            QString("%1:%2")
                .arg(*device->getComponentInstance().getName())
                .arg(padNumber);
        const QString value =
            AttributeSubstitutor::substitute(lookup("VALUE"), lookup)
                .simplified();
        const QString footprintName =
            *device->getLibPackage().getNames().getDefaultValue();
        gen.flashComponent(pad->getPosition(), pad->getRotation(), designator,
                           value, GerberGenerator::MountType::Fiducial,
                           QString(), QString(), footprintName);
        ++padNumber;
      }
    }
  }

  gen.generate();
  trackFileBeforeWrite(filePath);  // can throw
  gen.saveToFile(filePath);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardGerberExport::exportDrillsMerged(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixDrills());
  if (settings.getMergeDrillFiles()) {
    std::unique_ptr<ExcellonGenerator> gen =
        BoardGerberExport::createExcellonGenerator(
            settings, ExcellonGenerator::Plating::Mixed);
    drawPthDrills(*gen);
    drawNpthDrills(*gen);
    gen->generate();
    trackFileBeforeWrite(fp);  // can throw
    gen->saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportDrillsNpth(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixDrillsNpth());
  if (!settings.getMergeDrillFiles()) {
    std::unique_ptr<ExcellonGenerator> gen =
        BoardGerberExport::createExcellonGenerator(
            settings, ExcellonGenerator::Plating::No);
    drawNpthDrills(*gen);

    // Note that separate NPTH drill files could lead to issues with some PCB
    // manufacturers, even if it's empty in many cases. However, we generate the
    // NPTH file even if there are no NPTH drills since it could also lead to
    // unexpected behavior if the file is generated only conditionally. See
    // https://github.com/LibrePCB/LibrePCB/issues/998. If the PCB manufacturer
    // doesn't support a separate NPTH file, the user shall enable the
    // "merge PTH and NPTH drills"  option.
    gen->generate();
    trackFileBeforeWrite(fp);  // can throw
    gen->saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportDrillsPth(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixDrillsPth());
  if (!settings.getMergeDrillFiles()) {
    std::unique_ptr<ExcellonGenerator> gen =
        BoardGerberExport::createExcellonGenerator(
            settings, ExcellonGenerator::Plating::Yes);
    drawPthDrills(*gen);
    gen->generate();
    trackFileBeforeWrite(fp);  // can throw
    gen->saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportDrillsBlindBuried(
    const BoardFabricationOutputSettings& settings) const {
  auto vias = getBlindBuriedVias();
  for (auto it = vias.begin(); it != vias.end(); it++) {
    mCurrentStartLayer = it.key().first;
    mCurrentEndLayer = it.key().second;
    const FilePath fp = getOutputFilePath(
        settings.getOutputBasePath() % settings.getSuffixDrillsBlindBuried());
    std::unique_ptr<ExcellonGenerator> gen =
        BoardGerberExport::createExcellonGenerator(
            settings, ExcellonGenerator::Plating::Yes);
    foreach (const BI_Via* via, it.value()) {
      gen->drill(via->getPosition(), via->getDrillDiameter(), true,
                 ExcellonGenerator::Function::ViaDrill);
    }
    gen->generate();
    trackFileBeforeWrite(fp);  // can throw
    gen->saveToFile(fp);
  }
}

void BoardGerberExport::exportLayerBoardOutlines(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixOutlines());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      *mProject.getVersion());
  gen.setFileFunctionOutlines(false);
  drawLayer(gen, Layer::boardOutlines());
  drawLayer(gen, Layer::boardCutouts());
  gen.generate();
  trackFileBeforeWrite(fp);  // can throw
  gen.saveToFile(fp);
}

void BoardGerberExport::exportLayerTopCopper(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixCopperTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      *mProject.getVersion());
  gen.setFileFunctionCopper(1, GerberGenerator::CopperSide::Top,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, Layer::topCopper());
  gen.generate();
  trackFileBeforeWrite(fp);  // can throw
  gen.saveToFile(fp);
}

void BoardGerberExport::exportLayerBottomCopper(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixCopperBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      *mProject.getVersion());
  gen.setFileFunctionCopper(mBoard.getInnerLayerCount() + 2,
                            GerberGenerator::CopperSide::Bottom,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, Layer::botCopper());
  gen.generate();
  trackFileBeforeWrite(fp);  // can throw
  gen.saveToFile(fp);
}

void BoardGerberExport::exportLayerInnerCopper(
    const BoardFabricationOutputSettings& settings) const {
  for (int i = 1; i <= mBoard.getInnerLayerCount(); ++i) {
    mCurrentInnerCopperLayer = i;  // used for attribute provider
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixCopperInner());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionCopper(i + 1, GerberGenerator::CopperSide::Inner,
                              GerberGenerator::Polarity::Positive);
    if (const Layer* layer = Layer::innerCopper(i)) {
      drawLayer(gen, *layer);
    } else {
      throw LogicError(__FILE__, __LINE__, "Unknown inner copper layer.");
    }
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  }
  mCurrentInnerCopperLayer = 0;
}

void BoardGerberExport::exportLayerTopSolderMask(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixSolderMaskTop());
  if (mBoard.getSolderResist()) {
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Top,
                                  GerberGenerator::Polarity::Negative);
    drawLayer(gen, Layer::topStopMask());
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportLayerBottomSolderMask(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixSolderMaskBot());
  if (mBoard.getSolderResist()) {
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Bottom,
                                  GerberGenerator::Polarity::Negative);
    drawLayer(gen, Layer::botStopMask());
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportLayerTopSilkscreen(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixSilkscreenTop());
  const QVector<const Layer*>& layers = mBoard.getSilkscreenLayersTop();
  if (layers.count() > 0) {  // don't export silkscreen if no layers selected
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionLegend(GerberGenerator::BoardSide::Top,
                              GerberGenerator::Polarity::Positive);
    foreach (const Layer* layer, layers) {
      drawLayer(gen, *layer);
    }
    gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
    drawLayer(gen, Layer::topStopMask());
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportLayerBottomSilkscreen(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixSilkscreenBot());
  const QVector<const Layer*>& layers = mBoard.getSilkscreenLayersBot();
  if (layers.count() > 0) {  // don't export silkscreen if no layers selected
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionLegend(GerberGenerator::BoardSide::Bottom,
                              GerberGenerator::Polarity::Positive);
    foreach (const Layer* layer, layers) {
      drawLayer(gen, *layer);
    }
    gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
    drawLayer(gen, Layer::botStopMask());
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportLayerTopSolderPaste(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixSolderPasteTop());
  if (settings.getEnableSolderPasteTop()) {
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionPaste(GerberGenerator::BoardSide::Top,
                             GerberGenerator::Polarity::Positive);
    drawLayer(gen, Layer::topSolderPaste());
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

void BoardGerberExport::exportLayerBottomSolderPaste(
    const BoardFabricationOutputSettings& settings) const {
  const FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                        settings.getSuffixSolderPasteBot());
  if (settings.getEnableSolderPasteBot()) {
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        *mProject.getVersion());
    gen.setFileFunctionPaste(GerberGenerator::BoardSide::Bottom,
                             GerberGenerator::Polarity::Positive);
    drawLayer(gen, Layer::botSolderPaste());
    gen.generate();
    trackFileBeforeWrite(fp);  // can throw
    gen.saveToFile(fp);
  } else if (mRemoveObsoleteFiles && fp.isExistingFile() &&
             (!mWrittenFiles.contains(fp))) {
    FileUtils::removeFile(fp);
  }
}

int BoardGerberExport::drawNpthDrills(ExcellonGenerator& gen) const {
  int count = 0;

  // footprint holes
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      gen.drill(transform.map(hole.getPath()), hole.getDiameter(), false,
                ExcellonGenerator::Function::MechanicalDrill);
      ++count;
    }
  }

  // board holes
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    gen.drill(hole->getData().getPath(), hole->getData().getDiameter(), false,
              ExcellonGenerator::Function::MechanicalDrill);
    ++count;
  }

  return count;
}

int BoardGerberExport::drawPthDrills(ExcellonGenerator& gen) const {
  int count = 0;

  // footprint pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const FootprintPad& libPad = pad->getLibPad();
      const Transform transform(*pad);
      const ExcellonGenerator::Function function =
          (libPad.getFunction() == FootprintPad::Function::PressFitPad)
          ? ExcellonGenerator::Function::ComponentDrillPressFit
          : ExcellonGenerator::Function::ComponentDrill;
      for (const PadHole& hole : libPad.getHoles()) {
        gen.drill(transform.map(hole.getPath()), hole.getDiameter(), true,
                  function);  // can throw
        ++count;
      }
    }
  }

  // vias
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      if (via->getVia().isThrough()) {
        gen.drill(via->getPosition(), via->getDrillDiameter(), true,
                  ExcellonGenerator::Function::ViaDrill);
        ++count;
      }
    }
  }

  return count;
}

QMap<BoardGerberExport::LayerPair, QList<const BI_Via*>>
    BoardGerberExport::getBlindBuriedVias() const {
  QMap<LayerPair, QList<const BI_Via*>> result;
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      if (via->getVia().isBlind() || via->getVia().isBuried()) {
        if (auto span = via->getDrillLayerSpan()) {
          result[*span].append(via);
        }
      }
    }
  }
  return result;
}

void BoardGerberExport::drawLayer(GerberGenerator& gen,
                                  const Layer& layer) const {
  // draw footprints incl. pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Q_ASSERT(device);
    drawDevice(gen, *device, layer);
  }

  // draw all non-footprint objects
  drawLayerExceptDevices(gen, layer);
}

void BoardGerberExport::drawGlueLayer(GerberGenerator& gen, const Layer& layer,
                                      const Uuid& assemblyVariant) const {
  // draw footprints incl. pads (only those contained in the assembly variant)
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Q_ASSERT(device);
    if (device->isGlueEnabled() &&
        device->isInAssemblyVariant(assemblyVariant)) {
      drawDevice(gen, *device, layer);
    }
  }

  // draw all non-footprint objects
  drawLayerExceptDevices(gen, layer);
}

void BoardGerberExport::drawLayerExceptDevices(GerberGenerator& gen,
                                               const Layer& layer) const {
  // draw vias and traces (grouped by net)
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    Q_ASSERT(netsegment);
    QString net = netsegment->getNetSignal()
        ? *netsegment->getNetSignal()->getName()  // Named net.
        : "N/C";  // Anonymous net (reserved name by Gerber specs).
    foreach (const BI_Via* via, netsegment->getVias()) {
      Q_ASSERT(via);
      drawVia(gen, *via, layer, net);
    }
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      Q_ASSERT(netline);
      if (netline->getLayer() == layer) {
        gen.drawLine(
            netline->getP1().getPosition(), netline->getP2().getPosition(),
            positiveToUnsigned(netline->getWidth()),
            GerberAttribute::ApertureFunction::Conductor, net, QString());
      }
    }
  }

  // draw planes
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    Q_ASSERT(plane);
    if (plane->getLayer() == layer) {
      foreach (const Path& fragment, plane->getFragments()) {
        gen.drawPathArea(
            fragment, GerberAttribute::ApertureFunction::Conductor,
            plane->getNetSignal()
                ? std::make_optional(*plane->getNetSignal()->getName())
                : std::nullopt,
            QString());
      }
    }
  }

  // draw polygons
  GerberGenerator::Function graphicsFunction = std::nullopt;
  std::optional<QString> graphicsNet = std::nullopt;
  if (layer.isBoardEdge()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (layer.isCopper()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    Q_ASSERT(polygon);
    if (layer == polygon->getData().getLayer()) {
      drawPolygon(gen, layer, polygon->getData().getPath(),
                  polygon->getData().getLineWidth(),
                  polygon->getData().isFilled(), graphicsFunction, graphicsNet,
                  QString());
    }
  }

  // draw stroke texts
  GerberGenerator::Function textFunction = std::nullopt;
  if (layer.isCopper()) {
    textFunction = GerberAttribute::ApertureFunction::NonConductor;
  }
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    Q_ASSERT(text);
    if (layer == text->getData().getLayer()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getData().getStrokeWidth(), layer);
      const Transform transform(text->getData());
      foreach (Path path, transform.map(text->getPaths())) {
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            QString());
      }
    }
  }

  // Draw holes.
  if (layer.isStopMask()) {
    foreach (const BI_Hole* hole, mBoard.getHoles()) {
      if (const std::optional<Length>& offset = hole->getStopMaskOffset()) {
        const Length diameter =
            (*hole->getData().getDiameter()) + (*offset) + (*offset);
        const Path path = hole->getData().getPath()->cleaned();
        if (diameter > 0) {
          if (path.getVertices().count() == 1) {
            gen.flashCircle(path.getVertices().first().getPos(),
                            PositiveLength(diameter), std::nullopt,
                            std::nullopt, QString(), QString(), QString());
          } else {
            gen.drawPathOutline(path, UnsignedLength(diameter), std::nullopt,
                                std::nullopt, QString());
          }
        }
      }
    }
  }
}

void BoardGerberExport::drawVia(GerberGenerator& gen, const BI_Via& via,
                                const Layer& layer,
                                const QString& netName) const {
  const bool drawCopper = via.getVia().isOnLayer(layer);
  const std::optional<PositiveLength> stopMaskDiameter = layer.isStopMask()
      ? (layer.isTop() ? via.getStopMaskDiameterTop()
                       : via.getStopMaskDiameterBottom())
      : std::nullopt;
  if (drawCopper || stopMaskDiameter) {
    // Via attributes (only on copper layers).
    GerberGenerator::Function function = std::nullopt;
    std::optional<QString> net = std::nullopt;
    if (drawCopper) {
      function = GerberAttribute::ApertureFunction::ViaPad;
      net = netName;
    }

    const PositiveLength diameter =
        stopMaskDiameter ? (*stopMaskDiameter) : via.getSize();
    gen.flashCircle(via.getPosition(), diameter, function, net, QString(),
                    QString(), QString());
  }
}

void BoardGerberExport::drawDevice(GerberGenerator& gen,
                                   const BI_Device& device,
                                   const Layer& layer) const {
  GerberGenerator::Function graphicsFunction = std::nullopt;
  std::optional<QString> graphicsNet = std::nullopt;
  if (layer.isBoardEdge()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (layer.isCopper()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  QString component = *device.getComponentInstance().getName();

  // draw pads
  foreach (const BI_FootprintPad* pad, device.getPads()) {
    drawFootprintPad(gen, *pad, layer);
  }

  // draw polygons
  const Transform transform(device);
  for (const Polygon& polygon :
       device.getLibFootprint().getPolygons().sortedByUuid()) {
    const Layer& polygonLayer = transform.map(polygon.getLayer());
    if (polygonLayer == layer) {
      const Path path = transform.map(polygon.getPath());
      drawPolygon(gen, layer, path, polygon.getLineWidth(), polygon.isFilled(),
                  graphicsFunction, graphicsNet, component);
    }
  }

  // draw circles
  for (const Circle& circle :
       device.getLibFootprint().getCircles().sortedByUuid()) {
    const Layer& circleLayer = transform.map(circle.getLayer());
    if (circleLayer == layer) {
      Point absolutePos = transform.map(circle.getCenter());
      if (circle.isFilled()) {
        PositiveLength outerDia = circle.getDiameter() + circle.getLineWidth();
        gen.drawPathArea(Path::circle(outerDia).translated(absolutePos),
                         graphicsFunction, graphicsNet, component);
      } else {
        UnsignedLength lineWidth =
            calcWidthOfLayer(circle.getLineWidth(), circleLayer);
        gen.drawPathOutline(
            Path::circle(circle.getDiameter()).translated(absolutePos),
            lineWidth, graphicsFunction, graphicsNet, component);
      }
    }
  }

  // draw stroke texts (from footprint instance, *NOT* from library footprint!)
  GerberGenerator::Function textFunction = std::nullopt;
  if (layer.isCopper()) {
    textFunction = GerberAttribute::ApertureFunction::NonConductor;
  }
  foreach (const BI_StrokeText* text, device.getStrokeTexts()) {
    if (layer == text->getData().getLayer()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getData().getStrokeWidth(), layer);
      Transform transform(text->getData());
      foreach (Path path, transform.map(text->getPaths())) {
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            component);
      }
    }
  }

  // Draw holes.
  if (layer.isStopMask()) {
    for (const Hole& hole :
         device.getLibFootprint().getHoles().sortedByUuid()) {
      if (std::optional<Length> offset =
              device.getHoleStopMasks().value(hole.getUuid())) {
        const Length diameter = (*hole.getDiameter()) + (*offset) + (*offset);
        if (diameter > 0) {
          const Path path = transform.map(hole.getPath()->cleaned());
          if (path.getVertices().count() == 1) {
            gen.flashCircle(path.getVertices().first().getPos(),
                            PositiveLength(diameter), std::nullopt,
                            std::nullopt, QString(), QString(), QString());
          } else {
            gen.drawPathOutline(path, UnsignedLength(diameter), std::nullopt,
                                std::nullopt, QString());
          }
        }
      }
    }
  }
}

void BoardGerberExport::drawFootprintPad(GerberGenerator& gen,
                                         const BI_FootprintPad& pad,
                                         const Layer& layer) const {
  const QMap<FootprintPad::Function, GerberAttribute::ApertureFunction>
      functionMap = {
          {FootprintPad::Function::ThermalPad,
           GerberAttribute::ApertureFunction::HeatsinkPad},
          {FootprintPad::Function::BgaPad,
           GerberAttribute::ApertureFunction::BgaPadCopperDefined},
          {FootprintPad::Function::EdgeConnectorPad,
           GerberAttribute::ApertureFunction::ConnectorPad},
          {FootprintPad::Function::TestPad,
           GerberAttribute::ApertureFunction::TestPad},
          {FootprintPad::Function::LocalFiducial,
           GerberAttribute::ApertureFunction::FiducialPadLocal},
          {FootprintPad::Function::GlobalFiducial,
           GerberAttribute::ApertureFunction::FiducialPadGlobal},
      };

  foreach (const PadGeometry& geometry, pad.getGeometries().value(&layer)) {
    // Pad attributes (most of them only on copper layers).
    GerberGenerator::Function function = std::nullopt;
    std::optional<QString> net = std::nullopt;
    QString component = *pad.getDevice().getComponentInstance().getName();
    QString pin, signal;
    if (layer.isCopper()) {
      if (pad.getLibPad().isTht()) {
        function = GerberAttribute::ApertureFunction::ComponentPad;
      } else {
        function = GerberAttribute::ApertureFunction::SmdPadCopperDefined;
      }
      function = functionMap.value(pad.getLibPad().getFunction(), *function);
      net = pad.getCompSigInstNetSignal()
          ? *pad.getCompSigInstNetSignal()->getName()  // Named net.
          : "N/C";  // Anonymous net (reserved name by Gerber specs).
      if (const PackagePad* pkgPad = pad.getLibPackagePad()) {
        pin = *pkgPad->getName();
      }
      if (ComponentSignalInstance* cmpSig = pad.getComponentSignalInstance()) {
        signal = *cmpSig->getCompSignal().getName();
      }
    }

    // Helper to flash a custom outline by flattening all arcs.
    auto flashPadOutline = [&]() {
      foreach (Path outline, geometry.toOutlines()) {
        outline.flattenArcs(PositiveLength(5000));
        if (pad.getMirrored()) {
          outline.mirror(Qt::Horizontal);
        }
        gen.flashOutline(pad.getPosition(), StraightAreaPath(outline),
                         pad.getRotation(), function, net, component, pin,
                         signal);  // can throw
      }
    };

    // Flash shape.
    const Length width = geometry.getWidth();
    const Length height = geometry.getHeight();
    switch (geometry.getShape()) {
      case PadGeometry::Shape::RoundedRect: {
        if ((width > 0) && (height > 0)) {
          gen.flashRect(pad.getPosition(), PositiveLength(width),
                        PositiveLength(height), geometry.getCornerRadius(),
                        pad.getRotation(), function, net, component, pin,
                        signal);
        }
        break;
      }
      case PadGeometry::Shape::RoundedOctagon: {
        if ((width > 0) && (height > 0)) {
          gen.flashOctagon(pad.getPosition(), PositiveLength(width),
                           PositiveLength(height), geometry.getCornerRadius(),
                           pad.getRotation(), function, net, component, pin,
                           signal);
        }
        break;
      }
      case PadGeometry::Shape::Stroke: {
        if ((width > 0) && (!geometry.getPath().getVertices().isEmpty())) {
          const Transform transform(pad);
          const Path path = transform.map(geometry.getPath());
          if (path.getVertices().count() == 1) {
            // For maximum compatibility, convert the stroke to a circle.
            gen.flashCircle(path.getVertices().first().getPos(),
                            PositiveLength(width), function, net, component,
                            pin, signal);
          } else if ((path.getVertices().count() == 2) &&
                     (path.getVertices().first().getAngle() == 0)) {
            // For maximum compatibility, convert the stroke to an obround.
            const Point p0 = path.getVertices().at(0).getPos();
            const Point p1 = path.getVertices().at(1).getPos();
            const Point delta = p1 - p0;
            const Point center = (p0 + p1) / 2;
            const PositiveLength height(width);
            const PositiveLength width = height + (p1 - p0).getLength();
            const Angle rotation = Angle::fromRad(
                std::atan2(delta.getY().toMm(), delta.getX().toMm()));
            gen.flashObround(center, width, height, rotation, function, net,
                             component, pin, signal);
          } else {
            // As a last resort, convert the outlines to straight path segments
            // and flash them with outline apertures.
            flashPadOutline();  // can throw
          }
        }
        break;
      }
      case PadGeometry::Shape::Custom: {
        flashPadOutline();  // can throw
        break;
      }
      default: {
        throw LogicError(__FILE__, __LINE__, "Unknown pad shape!");
      }
    }
  }
}

void BoardGerberExport::drawPolygon(GerberGenerator& gen, const Layer& layer,
                                    const Path& outline,
                                    const UnsignedLength& lineWidth, bool fill,
                                    GerberGenerator::Function function,
                                    const std::optional<QString>& net,
                                    const QString& component) const {
  // Don't draw zero-width outlines if the path gets filled! They have
  // no purpose and Gerber states that zero-width strokes shall not be
  // created! However, if the path is not filled, let's draw the
  // outline anyway as this *might* lead to a warning during production
  // to inform the user about this shaky input data.
  if ((lineWidth > 0) || (!fill) || (!outline.isClosed())) {
    gen.drawPathOutline(outline, calcWidthOfLayer(lineWidth, layer), function,
                        net, component);
  }

  // Only fill closed paths (for consistency with the appearance in the
  // board editor, and because Gerber expects area outlines as closed).
  if (fill && outline.isClosed()) {
    gen.drawPathArea(outline, function, net, component);
  }
}

QVector<Path> BoardGerberExport::getComponentOutlines(
    const BI_Device& device, const Layer& layer) const {
  QVector<Path> result;
  const Transform transform(device);
  for (const Polygon& polygon :
       device.getLibFootprint().getPolygons().sortedByUuid()) {
    // Return only closed ones, sunce Gerber specs say that component outlines
    // must be closed.
    if ((!polygon.getLayer().getPolygonsRepresentAreas()) &&
        (!polygon.getPath().isClosed())) {
      continue;
    }
    if (polygon.isFilled()) {
      continue;
    }
    if (transform.map(polygon.getLayer()) != layer) {
      continue;
    }
    result.append(transform.map(polygon.getPathForRendering()));
  }
  for (const Circle& circle :
       device.getLibFootprint().getCircles().sortedByUuid()) {
    if (circle.isFilled()) {
      continue;
    }
    if (transform.map(circle.getLayer()) != layer) {
      continue;
    }
    result.append(transform.map(Path::circle(circle.getDiameter())));
  }
  return result;
}

std::unique_ptr<ExcellonGenerator> BoardGerberExport::createExcellonGenerator(
    const BoardFabricationOutputSettings& settings,
    ExcellonGenerator::Plating plating) const {
  std::unique_ptr<ExcellonGenerator> gen(new ExcellonGenerator(
      mCreationDateTime, mProjectName, mBoard.getUuid(), *mProject.getVersion(),
      plating, 1, mBoard.getInnerLayerCount() + 2));
  gen->setUseG85Slots(settings.getUseG85SlotCommand());
  return gen;
}

FilePath BoardGerberExport::getOutputFilePath(QString path) const noexcept {
  path = AttributeSubstitutor::substitute(
      path, [this](const QString& key) { return getAttributeValue(key); },
      [&](const QString& str) {
        return FilePath::cleanFileName(
            str, FilePath::ReplaceSpaces | FilePath::KeepCase);
      });

  if (QDir::isAbsolutePath(path)) {
    return FilePath(path);
  } else {
    return mBoard.getProject().getPath().getPathTo(path);
  }
}

QString BoardGerberExport::getAttributeValue(
    const QString& key) const noexcept {
  auto getLayerName = [](const Layer* layer) {
    Q_ASSERT(layer && layer->isCopper());
    if (layer->isTop()) {
      return QString("TOP");  // no tr()!
    } else if (layer->isBottom()) {
      return QString("BOTTOM");  // no tr()!
    } else {
      return QString("IN%1").arg(layer->getCopperNumber());  // no tr()!
    }
  };

  if ((key == QLatin1String("CU_LAYER")) && (mCurrentInnerCopperLayer > 0)) {
    return QString::number(mCurrentInnerCopperLayer);
  } else if ((mCurrentStartLayer) && (key == QLatin1String("START_LAYER"))) {
    return getLayerName(mCurrentStartLayer);
  } else if ((mCurrentEndLayer) && (key == QLatin1String("END_LAYER"))) {
    return getLayerName(mCurrentEndLayer);
  } else if ((mCurrentStartLayer) && (key == QLatin1String("START_NUMBER"))) {
    return QString::number(mCurrentStartLayer->getCopperNumber() + 1);
  } else if ((mCurrentEndLayer) && (key == QLatin1String("END_NUMBER"))) {
    return QString::number(mCurrentEndLayer->getCopperNumber() + 1);
  } else {
    const ProjectAttributeLookup lookup(mBoard, nullptr);
    return lookup(key);
  }
}

void BoardGerberExport::trackFileBeforeWrite(const FilePath& fp) const {
  if (mBeforeWriteCallback) {
    mBeforeWriteCallback(fp);  // can throw
  }
  mWrittenFiles.append(fp);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

UnsignedLength BoardGerberExport::calcWidthOfLayer(
    const UnsignedLength& width, const Layer& layer) noexcept {
  if ((layer.isBoardEdge()) && (width < UnsignedLength(1000))) {
    return UnsignedLength(1000);  // outlines should have a minimum width of 1um
  } else {
    return width;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
