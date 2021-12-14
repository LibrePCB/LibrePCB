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
#include "../../library/pkg/packagepad.h"
#include "../circuit/componentinstance.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "../projectmetadata.h"
#include "board.h"
#include "boarddesignrules.h"
#include "boardfabricationoutputsettings.h"
#include "boardlayerstack.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
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

BoardGerberExport::BoardGerberExport(
    const Board& board, const BoardFabricationOutputSettings& settings) noexcept
  : mProject(board.getProject()),
    mBoard(board),
    mSettings(new BoardFabricationOutputSettings(settings)),
    mCreationDateTime(QDateTime::currentDateTime()),
    mProjectName(*mProject.getMetadata().getName()),
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

FilePath BoardGerberExport::getOutputDirectory() const noexcept {
  return getOutputFilePath("dummy").getParentDir();  // use dummy suffix
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardGerberExport::exportAllLayers() const {
  mWrittenFiles.clear();

  if (mSettings->getMergeDrillFiles()) {
    exportDrills();
  } else {
    exportDrillsNpth();
    exportDrillsPth();
  }
  exportLayerBoardOutlines();
  exportLayerTopCopper();
  exportLayerInnerCopper();
  exportLayerBottomCopper();
  exportLayerTopSolderMask();
  exportLayerBottomSolderMask();
  exportLayerTopSilkscreen();
  exportLayerBottomSilkscreen();
  if (mSettings->getEnableSolderPasteTop()) {
    exportLayerTopSolderPaste();
  }
  if (mSettings->getEnableSolderPasteBot()) {
    exportLayerBottomSolderPaste();
  }
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

void BoardGerberExport::exportDrills() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixDrills());
  ExcellonGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getMetadata().getVersion(),
                        ExcellonGenerator::Plating::Mixed, 1,
                        mBoard.getLayerStack().getInnerLayerCount() + 2);
  drawPthDrills(gen);
  drawNpthDrills(gen);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportDrillsNpth() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixDrillsNpth());
  ExcellonGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getMetadata().getVersion(),
                        ExcellonGenerator::Plating::No, 1,
                        mBoard.getLayerStack().getInnerLayerCount() + 2);
  int count = drawNpthDrills(gen);
  if (count > 0) {
    // Some PCB manufacturers don't like to have separate drill files for PTH
    // and NPTH. As many boards don't have non-plated holes anyway, we create
    // this file only if it's really needed. Maybe this avoids unnecessary
    // issues with manufacturers...
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
}

void BoardGerberExport::exportDrillsPth() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixDrillsPth());
  ExcellonGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getMetadata().getVersion(),
                        ExcellonGenerator::Plating::Yes, 1,
                        mBoard.getLayerStack().getInnerLayerCount() + 2);
  drawPthDrills(gen);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBoardOutlines() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixOutlines());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
  gen.setFileFunctionOutlines(false);
  drawLayer(gen, GraphicsLayer::sBoardOutlines);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopCopper() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixCopperTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
  gen.setFileFunctionCopper(1, GerberGenerator::CopperSide::Top,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sTopCopper);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomCopper() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixCopperBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
  gen.setFileFunctionCopper(mBoard.getLayerStack().getInnerLayerCount() + 2,
                            GerberGenerator::CopperSide::Bottom,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sBotCopper);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerInnerCopper() const {
  for (int i = 1; i <= mBoard.getLayerStack().getInnerLayerCount(); ++i) {
    mCurrentInnerCopperLayer = i;  // used for attribute provider
    FilePath fp = getOutputFilePath(mSettings->getSuffixCopperInner());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getMetadata().getVersion());
    gen.setFileFunctionCopper(i + 1, GerberGenerator::CopperSide::Inner,
                              GerberGenerator::Polarity::Positive);
    drawLayer(gen, GraphicsLayer::getInnerLayerName(i));
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
  mCurrentInnerCopperLayer = 0;
}

void BoardGerberExport::exportLayerTopSolderMask() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixSolderMaskTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
  gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Top,
                                GerberGenerator::Polarity::Negative);
  drawLayer(gen, GraphicsLayer::sTopStopMask);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderMask() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixSolderMaskBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
  gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Bottom,
                                GerberGenerator::Polarity::Negative);
  drawLayer(gen, GraphicsLayer::sBotStopMask);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopSilkscreen() const {
  QStringList layers = mSettings->getSilkscreenLayersTop();
  if (layers.count() >
      0) {  // don't create silkscreen file if no layers selected
    FilePath fp = getOutputFilePath(mSettings->getSuffixSilkscreenTop());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getMetadata().getVersion());
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

void BoardGerberExport::exportLayerBottomSilkscreen() const {
  QStringList layers = mSettings->getSilkscreenLayersBot();
  if (layers.count() >
      0) {  // don't create silkscreen file if no layers selected
    FilePath fp = getOutputFilePath(mSettings->getSuffixSilkscreenBot());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getMetadata().getVersion());
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

void BoardGerberExport::exportLayerTopSolderPaste() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixSolderPasteTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
  gen.setFileFunctionPaste(GerberGenerator::BoardSide::Top,
                           GerberGenerator::Polarity::Positive);
  drawLayer(gen, GraphicsLayer::sTopSolderPaste);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderPaste() const {
  FilePath fp = getOutputFilePath(mSettings->getSuffixSolderPasteBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getMetadata().getVersion());
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
    const BI_Footprint& footprint = device->getFootprint();
    for (const Hole& hole : footprint.getLibFootprint().getHoles()) {
      gen.drill(footprint.mapToScene(hole.getPosition()), hole.getDiameter(),
                false, ExcellonGenerator::Function::MechanicalDrill);
      ++count;
    }
  }

  // board holes
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    gen.drill(hole->getHole().getPosition(), hole->getHole().getDiameter(),
              false, ExcellonGenerator::Function::MechanicalDrill);
    ++count;
  }

  return count;
}

int BoardGerberExport::drawPthDrills(ExcellonGenerator& gen) const {
  int count = 0;

  // footprint pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const BI_Footprint& footprint = device->getFootprint();
    foreach (const BI_FootprintPad* pad, footprint.getPads()) {
      const FootprintPad& libPad = pad->getLibPad();
      if (libPad.getBoardSide() == FootprintPad::BoardSide::THT) {
        gen.drill(pad->getPosition(),
                  PositiveLength(*libPad.getDrillDiameter()), true,
                  ExcellonGenerator::Function::ComponentDrill);  // can throw
        ++count;
      }
    }
  }

  // vias
  foreach (const BI_NetSegment* netsegment,
           sortedByUuid(mBoard.getNetSegments())) {
    foreach (const BI_Via* via, sortedByUuid(netsegment->getVias())) {
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
    drawFootprint(gen, device->getFootprint(), layerName);
  }

  // draw vias and traces (grouped by net)
  foreach (const BI_NetSegment* netsegment,
           sortedByUuid(mBoard.getNetSegments())) {
    Q_ASSERT(netsegment);
    QString net = netsegment->getNetSignal()
        ? *netsegment->getNetSignal()->getName()  // Named net.
        : "N/C";  // Anonymous net (reserved name by Gerber specs).
    foreach (const BI_Via* via, sortedByUuid(netsegment->getVias())) {
      Q_ASSERT(via);
      drawVia(gen, *via, layerName, net);
    }
    foreach (const BI_NetLine* netline,
             sortedByUuid(netsegment->getNetLines())) {
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
  foreach (const BI_Plane* plane, sortedByUuid(mBoard.getPlanes())) {
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
  foreach (const BI_Polygon* polygon, sortedByUuid(mBoard.getPolygons())) {
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
  foreach (const BI_StrokeText* text, sortedByUuid(mBoard.getStrokeTexts())) {
    Q_ASSERT(text);
    if (layerName == text->getText().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
      foreach (Path path, text->getText().getPaths()) {
        path.rotate(text->getText().getRotation());
        if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
        path.translate(text->getText().getPosition());
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
                      Angle::deg0(), function, net, QString(), QString(),
                      QString());
        break;
      }
      case Via::Shape::Octagon: {
        gen.flashOctagon(via.getPosition(), outerDiameter, outerDiameter,
                         Angle::deg0(), function, net, QString(), QString(),
                         QString());
        break;
      }
      default: { throw LogicError(__FILE__, __LINE__); }
    }
  }
}

void BoardGerberExport::drawFootprint(GerberGenerator& gen,
                                      const BI_Footprint& footprint,
                                      const QString& layerName) const {
  GerberGenerator::Function graphicsFunction = tl::nullopt;
  tl::optional<QString> graphicsNet = tl::nullopt;
  if (layerName == GraphicsLayer::sBoardOutlines) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (GraphicsLayer::isCopperLayer(layerName)) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  QString component =
      *footprint.getDeviceInstance().getComponentInstance().getName();

  // draw pads
  foreach (const BI_FootprintPad* pad, footprint.getPads()) {
    drawFootprintPad(gen, *pad, layerName);
  }

  // draw polygons
  for (const Polygon& polygon :
       footprint.getLibFootprint().getPolygons().sortedByUuid()) {
    QString layer = footprint.getIsMirrored()
        ? GraphicsLayer::getMirroredLayerName(layerName)
        : layerName;
    if (layer == polygon.getLayerName()) {
      Path path = polygon.getPath();
      path.rotate(footprint.getRotation());
      if (footprint.getIsMirrored()) path.mirror(Qt::Horizontal);
      path.translate(footprint.getPosition());
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
       footprint.getLibFootprint().getCircles().sortedByUuid()) {
    QString layer = footprint.getIsMirrored()
        ? GraphicsLayer::getMirroredLayerName(layerName)
        : layerName;
    if (layer == circle.getLayerName()) {
      Point absolutePos = circle.getCenter();
      absolutePos.rotate(footprint.getRotation());
      if (footprint.getIsMirrored()) absolutePos.mirror(Qt::Horizontal);
      absolutePos += footprint.getPosition();
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
  foreach (const BI_StrokeText* text,
           sortedByUuid(footprint.getStrokeTexts())) {
    if (layerName == text->getText().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
      foreach (Path path, text->getText().getPaths()) {
        path.rotate(text->getText().getRotation());
        if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
        path.translate(text->getPosition());
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            component);
      }
    }
  }
}

void BoardGerberExport::drawFootprintPad(GerberGenerator& gen,
                                         const BI_FootprintPad& pad,
                                         const QString& layerName) const {
  bool isSmt = pad.getLibPad().getBoardSide() != FootprintPad::BoardSide::THT;
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

  Angle rot = pad.getIsMirrored() ? -pad.getRotation() : pad.getRotation();
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
    qWarning() << "Pad with zero size ignored in gerber export:"
               << pad.getLibPadUuid();
    return;
  }

  PositiveLength pWidth(width);
  PositiveLength pHeight(height);

  // Pad attributes (most of them only on copper layers).
  GerberGenerator::Function function = tl::nullopt;
  tl::optional<QString> net = tl::nullopt;
  QString component =
      *pad.getFootprint().getDeviceInstance().getComponentInstance().getName();
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
      gen.flashObround(pad.getPosition(), pWidth, pHeight, rot, function, net,
                       component, pin, signal);
      break;
    }
    case FootprintPad::Shape::RECT: {
      gen.flashRect(pad.getPosition(), pWidth, pHeight, rot, function, net,
                    component, pin, signal);
      break;
    }
    case FootprintPad::Shape::OCTAGON: {
      gen.flashOctagon(pad.getPosition(), pWidth, pHeight, rot, function, net,
                       component, pin, signal);
      break;
    }
    default: { throw LogicError(__FILE__, __LINE__); }
  }
}

FilePath BoardGerberExport::getOutputFilePath(const QString& suffix) const
    noexcept {
  QString path = mSettings->getOutputBasePath() + suffix;
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
