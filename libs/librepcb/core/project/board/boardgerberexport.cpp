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
#include "../../geometry/hole.h"
#include "../../graphics/graphicslayer.h"
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
#include "board.h"
#include "boarddesignrules.h"
#include "boardfabricationoutputsettings.h"
#include "boardlayerstack.h"
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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardGerberExport::BoardGerberExport(const Board& board) noexcept
  : mProject(board.getProject()),
    mBoard(board),
    mCreationDateTime(QDateTime::currentDateTime()),
    mProjectName(*mProject.getName()),
    mCurrentInnerCopperLayer(0) {
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
 *  General Methods
 ******************************************************************************/

void BoardGerberExport::exportPcbLayers(
    const BoardFabricationOutputSettings& settings) const {
  mWrittenFiles.clear();

  if (settings.getMergeDrillFiles()) {
    exportDrills(settings);
  } else {
    exportDrillsNpth(settings);
    exportDrillsPth(settings);
  }
  exportLayerBoardOutlines(settings);
  exportLayerTopCopper(settings);
  exportLayerInnerCopper(settings);
  exportLayerBottomCopper(settings);
  exportLayerTopSolderMask(settings);
  exportLayerBottomSolderMask(settings);
  exportLayerTopSilkscreen(settings);
  exportLayerBottomSilkscreen(settings);
  if (settings.getEnableSolderPasteTop()) {
    exportLayerTopSolderPaste(settings);
  }
  if (settings.getEnableSolderPasteBot()) {
    exportLayerBottomSolderPaste(settings);
  }
}

void BoardGerberExport::exportComponentLayer(BoardSide side,
                                             const FilePath& filePath) const {
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  if (side == BoardSide::Top) {
    gen.setFileFunctionComponent(1, GerberGenerator::BoardSide::Top);
  } else {
    gen.setFileFunctionComponent(
        mBoard.getLayerStack().getInnerLayerCount() + 2,
        GerberGenerator::BoardSide::Bottom);
  }

  // Export board outline since this is useful for manual review.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    Q_ASSERT(polygon);
    if (polygon->getPolygon().getLayerName() == GraphicsLayer::sBoardOutlines) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(polygon->getPolygon().getLineWidth(),
                           *polygon->getPolygon().getLayerName());
      gen.drawPathOutline(polygon->getPolygon().getPath(), lineWidth,
                          GerberAttribute::ApertureFunction::Profile,
                          tl::nullopt, QString());
    }
  }

  // Export all components on the selected board side.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    if (device->getMirrored() == (side == BoardSide::Bottom)) {
      // Skip devices which are considered as no device to be mounted.
      GerberGenerator::MountType mountType = GerberGenerator::MountType::Other;
      switch (device->determineMountType()) {
        case BI_Device::MountType::Tht:
          mountType = GerberGenerator::MountType::Tht;
          break;
        case BI_Device::MountType::Smt:
          mountType = GerberGenerator::MountType::Smt;
          break;
        case BI_Device::MountType::Fiducial:
          mountType = GerberGenerator::MountType::Fiducial;
          break;
        case BI_Device::MountType::None:
          continue;
        default:
          break;
      }

      // Export component center and attributes.
      Angle rotation = device->getMirrored() ? -device->getRotation()
                                             : device->getRotation();
      QString designator = *device->getComponentInstance().getName();
      QString value = device->getComponentInstance().getValue(true).trimmed();
      QString manufacturer =
          AttributeSubstitutor::substitute("{{MANUFACTURER}}", device)
              .trimmed();
      QString mpn = AttributeSubstitutor::substitute(
                        "{{MPN or PARTNUMBER or DEVICE}}", device)
                        .trimmed();
      // Note: Always use english locale to make PnP files portable.
      QString footprintName =
          *device->getLibPackage().getNames().getDefaultValue();
      gen.flashComponent(device->getPosition(), rotation, designator, value,
                         mountType, manufacturer, mpn, footprintName);

      // Export component outline. But only closed ones, sunce Gerber specs say
      // that component outlines must be closed.
      QHash<QString, GerberAttribute::ApertureFunction> layerFunction;
      if (side == BoardSide::Top) {
        layerFunction[GraphicsLayer::sTopDocumentation] =
            GerberAttribute::ApertureFunction::ComponentOutlineBody;
        layerFunction[GraphicsLayer::sTopCourtyard] =
            GerberAttribute::ApertureFunction::ComponentOutlineCourtyard;
      } else {
        layerFunction[GraphicsLayer::sBotDocumentation] =
            GerberAttribute::ApertureFunction::ComponentOutlineBody;
        layerFunction[GraphicsLayer::sBotCourtyard] =
            GerberAttribute::ApertureFunction::ComponentOutlineCourtyard;
      }
      const Transform transform(*device);
      for (const Polygon& polygon :
           device->getLibFootprint().getPolygons().sortedByUuid()) {
        if (!polygon.getPath().isClosed()) {
          continue;
        }
        if (polygon.isFilled()) {
          continue;
        }
        QString layer = transform.map(*polygon.getLayerName());
        if (!layerFunction.contains(layer)) {
          continue;
        }
        Path path = transform.map(polygon.getPath());
        gen.drawComponentOutline(path, rotation, designator, value, mountType,
                                 manufacturer, mpn, footprintName,
                                 layerFunction[layer]);
      }

      // Export component pins.
      foreach (const BI_FootprintPad* pad, device->getPads()) {
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

  gen.generate();
  gen.saveToFile(filePath);
  mWrittenFiles.append(filePath);
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString BoardGerberExport::getBuiltInAttributeValue(const QString& key) const
    noexcept {
  if ((key == QLatin1String("CU_LAYER")) && (mCurrentInnerCopperLayer > 0)) {
    return QString::number(mCurrentInnerCopperLayer);
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*>
    BoardGerberExport::getAttributeProviderParents() const noexcept {
  return QVector<const AttributeProvider*>{&mBoard};
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardGerberExport::exportDrills(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixDrills());
  std::unique_ptr<ExcellonGenerator> gen =
      BoardGerberExport::createExcellonGenerator(
          settings, ExcellonGenerator::Plating::Mixed);
  drawPthDrills(*gen);
  drawNpthDrills(*gen);
  gen->generate();
  gen->saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportDrillsNpth(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixDrillsNpth());
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
  gen->saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportDrillsPth(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixDrillsPth());
  std::unique_ptr<ExcellonGenerator> gen =
      BoardGerberExport::createExcellonGenerator(
          settings, ExcellonGenerator::Plating::Yes);
  drawPthDrills(*gen);
  gen->generate();
  gen->saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBoardOutlines(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixOutlines());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionOutlines(false);
  drawLayer(gen, GraphicsLayer::sBoardOutlines);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopCopper(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixCopperTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionCopper(1, GerberGenerator::CopperSide::Top,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sTopCopper);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomCopper(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixCopperBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionCopper(mBoard.getLayerStack().getInnerLayerCount() + 2,
                            GerberGenerator::CopperSide::Bottom,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sBotCopper);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerInnerCopper(
    const BoardFabricationOutputSettings& settings) const {
  for (int i = 1; i <= mBoard.getLayerStack().getInnerLayerCount(); ++i) {
    mCurrentInnerCopperLayer = i;  // used for attribute provider
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixCopperInner());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getVersion());
    gen.setFileFunctionCopper(i + 1, GerberGenerator::CopperSide::Inner,
                              GerberGenerator::Polarity::Positive);
    drawLayer(gen, GraphicsLayer::getInnerLayerName(i));
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
  mCurrentInnerCopperLayer = 0;
}

void BoardGerberExport::exportLayerTopSolderMask(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderMaskTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Top,
                                GerberGenerator::Polarity::Negative);
  drawLayer(gen, GraphicsLayer::sTopStopMask);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderMask(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderMaskBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Bottom,
                                GerberGenerator::Polarity::Negative);
  drawLayer(gen, GraphicsLayer::sBotStopMask);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopSilkscreen(
    const BoardFabricationOutputSettings& settings) const {
  QStringList layers = settings.getSilkscreenLayersTop();
  if (layers.count() >
      0) {  // don't create silkscreen file if no layers selected
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixSilkscreenTop());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getVersion());
    gen.setFileFunctionLegend(GerberGenerator::BoardSide::Top,
                              GerberGenerator::Polarity::Positive);
    foreach (const QString& layer, layers) { drawLayer(gen, layer); }
    gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
    drawLayer(gen, GraphicsLayer::sTopStopMask);
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
}

void BoardGerberExport::exportLayerBottomSilkscreen(
    const BoardFabricationOutputSettings& settings) const {
  QStringList layers = settings.getSilkscreenLayersBot();
  if (layers.count() >
      0) {  // don't create silkscreen file if no layers selected
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixSilkscreenBot());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getVersion());
    gen.setFileFunctionLegend(GerberGenerator::BoardSide::Bottom,
                              GerberGenerator::Polarity::Positive);
    foreach (const QString& layer, layers) { drawLayer(gen, layer); }
    gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
    drawLayer(gen, GraphicsLayer::sBotStopMask);
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
}

void BoardGerberExport::exportLayerTopSolderPaste(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderPasteTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionPaste(GerberGenerator::BoardSide::Top,
                           GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sTopSolderPaste);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderPaste(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderPasteBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionPaste(GerberGenerator::BoardSide::Bottom,
                           GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sBotSolderPaste);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
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
    gen.drill(hole->getHole().getPath(), hole->getHole().getDiameter(), false,
              ExcellonGenerator::Function::MechanicalDrill);
    ++count;
  }

  return count;
}

int BoardGerberExport::drawPthDrills(ExcellonGenerator& gen) const {
  int count = 0;

  // footprint pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform deviceTransform(*device);
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const FootprintPad& libPad = pad->getLibPad();
      const Transform padTransform(libPad.getPosition(), libPad.getRotation());
      for (const Hole& hole : libPad.getHoles()) {
        gen.drill(deviceTransform.map(padTransform.map(hole.getPath())),
                  hole.getDiameter(), true,
                  ExcellonGenerator::Function::ComponentDrill);  // can throw
        ++count;
      }
    }
  }

  // vias
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      gen.drill(via->getPosition(), via->getDrillDiameter(), true,
                ExcellonGenerator::Function::ViaDrill);
      ++count;
    }
  }

  return count;
}

void BoardGerberExport::drawLayer(GerberGenerator& gen,
                                  const QString& layerName) const {
  // draw footprints incl. pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Q_ASSERT(device);
    drawDevice(gen, *device, layerName);
  }

  // draw vias and traces (grouped by net)
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    Q_ASSERT(netsegment);
    QString net = netsegment->getNetSignal()
        ? *netsegment->getNetSignal()->getName()  // Named net.
        : "N/C";  // Anonymous net (reserved name by Gerber specs).
    foreach (const BI_Via* via, netsegment->getVias()) {
      Q_ASSERT(via);
      drawVia(gen, *via, layerName, net);
    }
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      Q_ASSERT(netline);
      if (netline->getLayer().getName() == layerName) {
        gen.drawLine(netline->getStartPoint().getPosition(),
                     netline->getEndPoint().getPosition(),
                     positiveToUnsigned(netline->getWidth()),
                     GerberAttribute::ApertureFunction::Conductor, net,
                     QString());
      }
    }
  }

  // draw planes
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    Q_ASSERT(plane);
    if (plane->getLayerName() == layerName) {
      foreach (const Path& fragment, plane->getFragments()) {
        gen.drawPathArea(fragment, GerberAttribute::ApertureFunction::Conductor,
                         *plane->getNetSignal().getName(), QString());
      }
    }
  }

  // draw polygons
  GerberGenerator::Function graphicsFunction = tl::nullopt;
  tl::optional<QString> graphicsNet = tl::nullopt;
  if (layerName == GraphicsLayer::sBoardOutlines) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (GraphicsLayer::isCopperLayer(layerName)) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    Q_ASSERT(polygon);
    if (layerName == polygon->getPolygon().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(polygon->getPolygon().getLineWidth(), layerName);
      gen.drawPathOutline(polygon->getPolygon().getPath(), lineWidth,
                          graphicsFunction, graphicsNet, QString());
      // Only fill closed paths (for consistency with the appearance in the
      // board editor, and because Gerber expects area outlines as closed).
      if (polygon->getPolygon().isFilled() &&
          polygon->getPolygon().getPath().isClosed()) {
        gen.drawPathArea(polygon->getPolygon().getPath(), graphicsFunction,
                         graphicsNet, QString());
      }
    }
  }

  // draw stroke texts
  GerberGenerator::Function textFunction = tl::nullopt;
  if (GraphicsLayer::isCopperLayer(layerName)) {
    textFunction = GerberAttribute::ApertureFunction::NonConductor;
  }
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    Q_ASSERT(text);
    if (layerName == text->getText().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
      const Transform transform(text->getText());
      foreach (Path path, transform.map(text->generatePaths())) {
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            QString());
      }
    }
  }
}

void BoardGerberExport::drawVia(GerberGenerator& gen, const BI_Via& via,
                                const QString& layerName,
                                const QString& netName) const {
  bool drawCopper = via.isOnLayer(layerName);
  bool drawStopMask = (layerName == GraphicsLayer::sTopStopMask ||
                       layerName == GraphicsLayer::sBotStopMask) &&
      mBoard.getDesignRules().doesViaRequireStopMask(*via.getDrillDiameter());
  if (drawCopper || drawStopMask) {
    PositiveLength outerDiameter = via.getSize();
    if (drawStopMask) {
      outerDiameter += UnsignedLength(
          mBoard.getDesignRules().calcStopMaskClearance(*via.getSize()) * 2);
    }

    // Via attributes (only on copper layers).
    GerberGenerator::Function function = tl::nullopt;
    tl::optional<QString> net = tl::nullopt;
    if (drawCopper) {
      function = GerberAttribute::ApertureFunction::ViaPad;
      net = netName;
    }

    switch (via.getShape()) {
      case Via::Shape::Round: {
        gen.flashCircle(via.getPosition(), outerDiameter, function, net,
                        QString(), QString(), QString());
        break;
      }
      case Via::Shape::Square: {
        gen.flashRect(via.getPosition(), outerDiameter, outerDiameter,
                      UnsignedLength(0), Angle::deg0(), function, net,
                      QString(), QString(), QString());
        break;
      }
      case Via::Shape::Octagon: {
        gen.flashOctagon(via.getPosition(), outerDiameter, outerDiameter,
                         UnsignedLength(0), Angle::deg0(), function, net,
                         QString(), QString(), QString());
        break;
      }
      default: { throw LogicError(__FILE__, __LINE__); }
    }
  }
}

void BoardGerberExport::drawDevice(GerberGenerator& gen,
                                   const BI_Device& device,
                                   const QString& layerName) const {
  GerberGenerator::Function graphicsFunction = tl::nullopt;
  tl::optional<QString> graphicsNet = tl::nullopt;
  if (layerName == GraphicsLayer::sBoardOutlines) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (GraphicsLayer::isCopperLayer(layerName)) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  QString component = *device.getComponentInstance().getName();

  // draw pads
  foreach (const BI_FootprintPad* pad, device.getPads()) {
    drawFootprintPad(gen, *pad, layerName);
  }

  // draw polygons
  const Transform transform(device);
  for (const Polygon& polygon :
       device.getLibFootprint().getPolygons().sortedByUuid()) {
    QString layer = transform.map(layerName);
    if (layer == polygon.getLayerName()) {
      Path path = transform.map(polygon.getPath());
      gen.drawPathOutline(path, calcWidthOfLayer(polygon.getLineWidth(), layer),
                          graphicsFunction, graphicsNet, component);
      // Only fill closed paths (for consistency with the appearance in the
      // board editor, and because Gerber expects area outlines as closed).
      if (polygon.isFilled() && path.isClosed()) {
        gen.drawPathArea(path, graphicsFunction, graphicsNet, component);
      }
    }
  }

  // draw circles
  for (const Circle& circle :
       device.getLibFootprint().getCircles().sortedByUuid()) {
    QString layer = transform.map(layerName);
    if (layer == circle.getLayerName()) {
      Point absolutePos = transform.map(circle.getCenter());
      if (circle.isFilled()) {
        PositiveLength outerDia = circle.getDiameter() + circle.getLineWidth();
        gen.drawPathArea(Path::circle(outerDia).translated(absolutePos),
                         graphicsFunction, graphicsNet, component);
      } else {
        UnsignedLength lineWidth =
            calcWidthOfLayer(circle.getLineWidth(), layer);
        gen.drawPathOutline(
            Path::circle(circle.getDiameter()).translated(absolutePos),
            lineWidth, graphicsFunction, graphicsNet, component);
      }
    }
  }

  // draw stroke texts (from footprint instance, *NOT* from library footprint!)
  GerberGenerator::Function textFunction = tl::nullopt;
  if (GraphicsLayer::isCopperLayer(layerName)) {
    textFunction = GerberAttribute::ApertureFunction::NonConductor;
  }
  foreach (const BI_StrokeText* text, device.getStrokeTexts()) {
    if (layerName == text->getText().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
      Transform transform(text->getText());
      foreach (Path path, transform.map(text->generatePaths())) {
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            component);
      }
    }
  }
}

void BoardGerberExport::drawFootprintPad(GerberGenerator& gen,
                                         const BI_FootprintPad& pad,
                                         const QString& layerName) const {
  bool isSmt = !pad.getLibPad().isTht();
  bool isOnCopperLayer = pad.isOnLayer(layerName);
  bool isOnSolderMaskTop = pad.isOnLayer(GraphicsLayer::sTopCopper) &&
      (layerName == GraphicsLayer::sTopStopMask);
  bool isOnSolderMaskBottom = pad.isOnLayer(GraphicsLayer::sBotCopper) &&
      (layerName == GraphicsLayer::sBotStopMask);
  bool isOnSolderPasteTop = isSmt && pad.isOnLayer(GraphicsLayer::sTopCopper) &&
      (layerName == GraphicsLayer::sTopSolderPaste);
  bool isOnSolderPasteBottom = isSmt &&
      pad.isOnLayer(GraphicsLayer::sBotCopper) &&
      (layerName == GraphicsLayer::sBotSolderPaste);
  if (!isOnCopperLayer && !isOnSolderMaskTop && !isOnSolderMaskBottom &&
      !isOnSolderPasteTop && !isOnSolderPasteBottom) {
    return;
  }

  const FootprintPad& libPad = pad.getLibPad();
  Length width = *libPad.getWidth();
  Length height = *libPad.getHeight();
  if (isOnSolderMaskTop || isOnSolderMaskBottom) {
    Length size = qMin(width, height);
    UnsignedLength clearance =
        mBoard.getDesignRules().calcStopMaskClearance(size);
    width += clearance * 2;
    height += clearance * 2;
  } else if (isOnSolderPasteTop || isOnSolderPasteBottom) {
    Length size = qMin(width, height);
    Length clearance = -mBoard.getDesignRules().calcCreamMaskClearance(size);
    width += clearance * 2;
    height += clearance * 2;
  }

  if ((width <= 0) || (height <= 0)) {
    qWarning() << "Pad with zero size ignored in Gerber export:"
               << pad.getLibPadUuid();
    return;
  }

  PositiveLength pWidth(width);
  PositiveLength pHeight(height);

  // Pad attributes (most of them only on copper layers).
  GerberGenerator::Function function = tl::nullopt;
  tl::optional<QString> net = tl::nullopt;
  QString component = *pad.getDevice().getComponentInstance().getName();
  QString pin, signal;
  if (isOnCopperLayer) {
    if (!isSmt) {
      function = GerberAttribute::ApertureFunction::ComponentPad;
    } else {
      function = GerberAttribute::ApertureFunction::SmdPadCopperDefined;
    }
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

  switch (libPad.getShape()) {
    case FootprintPad::Shape::ROUND: {
      gen.flashObround(pad.getPosition(), pWidth, pHeight, pad.getRotation(),
                       function, net, component, pin, signal);
      break;
    }
    case FootprintPad::Shape::RECT: {
      gen.flashRect(pad.getPosition(), pWidth, pHeight, UnsignedLength(0),
                    pad.getRotation(), function, net, component, pin, signal);
      break;
    }
    case FootprintPad::Shape::OCTAGON: {
      gen.flashOctagon(pad.getPosition(), pWidth, pHeight, UnsignedLength(0),
                       pad.getRotation(), function, net, component, pin,
                       signal);
      break;
    }
    default: { throw LogicError(__FILE__, __LINE__); }
  }
}

std::unique_ptr<ExcellonGenerator> BoardGerberExport::createExcellonGenerator(
    const BoardFabricationOutputSettings& settings,
    ExcellonGenerator::Plating plating) const {
  std::unique_ptr<ExcellonGenerator> gen(new ExcellonGenerator(
      mCreationDateTime, mProjectName, mBoard.getUuid(), mProject.getVersion(),
      plating, 1, mBoard.getLayerStack().getInnerLayerCount() + 2));
  gen->setUseG85Slots(settings.getUseG85SlotCommand());
  return gen;
}

FilePath BoardGerberExport::getOutputFilePath(QString path) const noexcept {
  path = AttributeSubstitutor::substitute(path, this, [&](const QString& str) {
    return FilePath::cleanFileName(
        str, FilePath::ReplaceSpaces | FilePath::KeepCase);
  });

  if (QDir::isAbsolutePath(path)) {
    return FilePath(path);
  } else {
    return mBoard.getProject().getPath().getPathTo(path);
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

UnsignedLength BoardGerberExport::calcWidthOfLayer(
    const UnsignedLength& width, const QString& name) noexcept {
  if ((name == GraphicsLayer::sBoardOutlines) &&
      (width < UnsignedLength(1000))) {
    return UnsignedLength(1000);  // outlines should have a minimum width of 1um
  } else {
    return width;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
