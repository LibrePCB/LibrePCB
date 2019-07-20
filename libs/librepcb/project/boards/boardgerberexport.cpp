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

#include "../metadata/projectmetadata.h"
#include "../project.h"
#include "board.h"
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

#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/common/boarddesignrules.h>
#include <librepcb/common/cam/excellongenerator.h>
#include <librepcb/common/cam/gerbergenerator.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintpad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardGerberExport::BoardGerberExport(
    const Board& board, const BoardFabricationOutputSettings& settings) noexcept
  : mProject(board.getProject()),
    mBoard(board),
    mSettings(new BoardFabricationOutputSettings(settings)),
    mCurrentInnerCopperLayer(0) {
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
  FilePath          fp = getOutputFilePath(mSettings->getSuffixDrills());
  ExcellonGenerator gen;
  drawPthDrills(gen);
  drawNpthDrills(gen);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportDrillsNpth() const {
  FilePath          fp = getOutputFilePath(mSettings->getSuffixDrillsNpth());
  ExcellonGenerator gen;
  int               count = drawNpthDrills(gen);
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
  FilePath          fp = getOutputFilePath(mSettings->getSuffixDrillsPth());
  ExcellonGenerator gen;
  drawPthDrills(gen);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBoardOutlines() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixOutlines());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
  drawLayer(gen, GraphicsLayer::sBoardOutlines);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopCopper() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixCopperTop());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
  drawLayer(gen, GraphicsLayer::sTopCopper);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomCopper() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixCopperBot());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
  drawLayer(gen, GraphicsLayer::sBotCopper);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerInnerCopper() const {
  for (int i = 1; i <= mBoard.getLayerStack().getInnerLayerCount(); ++i) {
    mCurrentInnerCopperLayer = i;  // used for attribute provider
    FilePath        fp = getOutputFilePath(mSettings->getSuffixCopperInner());
    GerberGenerator gen(
        mProject.getMetadata().getName() % " - " % mBoard.getName(),
        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::getInnerLayerName(i));
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
  mCurrentInnerCopperLayer = 0;
}

void BoardGerberExport::exportLayerTopSolderMask() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixSolderMaskTop());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
  drawLayer(gen, GraphicsLayer::sTopStopMask);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderMask() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixSolderMaskBot());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
  drawLayer(gen, GraphicsLayer::sBotStopMask);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopSilkscreen() const {
  QStringList layers = mSettings->getSilkscreenLayersTop();
  if (layers.count() >
      0) {  // don't create silkscreen file if no layers selected
    FilePath        fp = getOutputFilePath(mSettings->getSuffixSilkscreenTop());
    GerberGenerator gen(
        mProject.getMetadata().getName() % " - " % mBoard.getName(),
        mBoard.getUuid(), mProject.getMetadata().getVersion());
    foreach (const QString& layer, layers) { drawLayer(gen, layer); }
    gen.setLayerPolarity(GerberGenerator::LayerPolarity::Negative);
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
    FilePath        fp = getOutputFilePath(mSettings->getSuffixSilkscreenBot());
    GerberGenerator gen(
        mProject.getMetadata().getName() % " - " % mBoard.getName(),
        mBoard.getUuid(), mProject.getMetadata().getVersion());
    foreach (const QString& layer, layers) { drawLayer(gen, layer); }
    gen.setLayerPolarity(GerberGenerator::LayerPolarity::Negative);
    drawLayer(gen, GraphicsLayer::sBotStopMask);
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
}

void BoardGerberExport::exportLayerTopSolderPaste() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixSolderPasteTop());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
  drawLayer(gen, GraphicsLayer::sTopSolderPaste);
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderPaste() const {
  FilePath        fp = getOutputFilePath(mSettings->getSuffixSolderPasteBot());
  GerberGenerator gen(
      mProject.getMetadata().getName() % " - " % mBoard.getName(),
      mBoard.getUuid(), mProject.getMetadata().getVersion());
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
      gen.drill(footprint.mapToScene(hole.getPosition()), hole.getDiameter());
      ++count;
    }
  }

  // board holes
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    gen.drill(hole->getHole().getPosition(), hole->getHole().getDiameter());
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
      const library::FootprintPad& libPad = pad->getLibPad();
      if (libPad.getBoardSide() == library::FootprintPad::BoardSide::THT) {
        gen.drill(pad->getPosition(),
                  PositiveLength(*libPad.getDrillDiameter()));  // can throw
        ++count;
      }
    }
  }

  // vias
  foreach (const BI_NetSegment* netsegment,
           sortedByUuid(mBoard.getNetSegments())) {
    foreach (const BI_Via* via, sortedByUuid(netsegment->getVias())) {
      gen.drill(via->getPosition(), via->getDrillDiameter());
      ++count;
    }
  }

  return count;
}

void BoardGerberExport::drawLayer(GerberGenerator& gen,
                                  const QString&   layerName) const {
  // draw footprints incl. pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Q_ASSERT(device);
    drawFootprint(gen, device->getFootprint(), layerName);
  }

  // draw vias
  foreach (const BI_NetSegment* netsegment,
           sortedByUuid(mBoard.getNetSegments())) {
    Q_ASSERT(netsegment);
    foreach (const BI_Via* via, sortedByUuid(netsegment->getVias())) {
      Q_ASSERT(via);
      drawVia(gen, *via, layerName);
    }
  }

  // draw traces
  foreach (const BI_NetSegment* netsegment,
           sortedByUuid(mBoard.getNetSegments())) {
    Q_ASSERT(netsegment);
    foreach (const BI_NetLine* netline,
             sortedByUuid(netsegment->getNetLines())) {
      Q_ASSERT(netline);
      if (netline->getLayer().getName() == layerName) {
        gen.drawLine(netline->getStartPoint().getPosition(),
                     netline->getEndPoint().getPosition(),
                     positiveToUnsigned(netline->getWidth()));
      }
    }
  }

  // draw planes
  foreach (const BI_Plane* plane, sortedByUuid(mBoard.getPlanes())) {
    Q_ASSERT(plane);
    if (plane->getLayerName() == layerName) {
      foreach (const Path& fragment, plane->getFragments()) {
        gen.drawPathArea(fragment);
      }
    }
  }

  // draw polygons
  foreach (const BI_Polygon* polygon, sortedByUuid(mBoard.getPolygons())) {
    Q_ASSERT(polygon);
    if (layerName == polygon->getPolygon().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(polygon->getPolygon().getLineWidth(), layerName);
      gen.drawPathOutline(polygon->getPolygon().getPath(), lineWidth);
      // Only fill closed paths (for consistency with the appearance in the
      // board editor, and because Gerber expects area outlines as closed).
      if (polygon->getPolygon().isFilled() &&
          polygon->getPolygon().getPath().isClosed()) {
        gen.drawPathArea(polygon->getPolygon().getPath());
      }
    }
  }

  // draw stroke texts
  foreach (const BI_StrokeText* text, sortedByUuid(mBoard.getStrokeTexts())) {
    Q_ASSERT(text);
    if (layerName == text->getText().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
      foreach (Path path, text->getText().getPaths()) {
        path.rotate(text->getText().getRotation());
        if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
        path.translate(text->getText().getPosition());
        gen.drawPathOutline(path, lineWidth);
      }
    }
  }
}

void BoardGerberExport::drawVia(GerberGenerator& gen, const BI_Via& via,
                                const QString& layerName) const {
  bool drawCopper = via.isOnLayer(layerName);
  bool drawStopMask =
      (layerName == GraphicsLayer::sTopStopMask ||
       layerName == GraphicsLayer::sBotStopMask) &&
      mBoard.getDesignRules().doesViaRequireStopMask(*via.getDrillDiameter());
  if (drawCopper || drawStopMask) {
    UnsignedLength outerDiameter = positiveToUnsigned(via.getSize());
    if (drawStopMask) {
      outerDiameter += UnsignedLength(
          mBoard.getDesignRules().calcStopMaskClearance(*via.getSize()) * 2);
    }
    switch (via.getShape()) {
      case BI_Via::Shape::Round: {
        gen.flashCircle(via.getPosition(), outerDiameter, UnsignedLength(0));
        break;
      }
      case BI_Via::Shape::Square: {
        gen.flashRect(via.getPosition(), outerDiameter, outerDiameter,
                      Angle::deg0(), UnsignedLength(0));
        break;
      }
      case BI_Via::Shape::Octagon: {
        gen.flashRegularPolygon(via.getPosition(), outerDiameter, 8,
                                Angle::deg0(), UnsignedLength(0));
        break;
      }
      default: { throw LogicError(__FILE__, __LINE__); }
    }
  }
}

void BoardGerberExport::drawFootprint(GerberGenerator&    gen,
                                      const BI_Footprint& footprint,
                                      const QString&      layerName) const {
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
      gen.drawPathOutline(path,
                          calcWidthOfLayer(polygon.getLineWidth(), layer));
      // Only fill closed paths (for consistency with the appearance in the
      // board editor, and because Gerber expects area outlines as closed).
      if (polygon.isFilled() && path.isClosed()) {
        gen.drawPathArea(path);
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
      Circle e = circle;
      if (footprint.getIsMirrored())
        e.setCenter(e.getCenter().mirrored(Qt::Horizontal));
      e.setCenter(e.getCenter() + footprint.getPosition());
      e.setLineWidth(calcWidthOfLayer(e.getLineWidth(), layer));
      gen.drawCircleOutline(e);
      if (e.isFilled()) {
        gen.drawCircleArea(e);
      }
    }
  }

  // draw stroke texts (from footprint instance, *NOT* from library footprint!)
  foreach (const BI_StrokeText* text,
           sortedByUuid(footprint.getStrokeTexts())) {
    if (layerName == text->getText().getLayerName()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
      foreach (Path path, text->getText().getPaths()) {
        path.rotate(text->getText().getRotation());
        if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
        path.translate(text->getPosition());
        gen.drawPathOutline(path, lineWidth);
      }
    }
  }
}

void BoardGerberExport::drawFootprintPad(GerberGenerator&       gen,
                                         const BI_FootprintPad& pad,
                                         const QString& layerName) const {
  bool isSmt =
      pad.getLibPad().getBoardSide() != library::FootprintPad::BoardSide::THT;
  bool isOnCopperLayer   = pad.isOnLayer(layerName);
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
  const library::FootprintPad& libPad = pad.getLibPad();
  Length                       width  = *libPad.getWidth();
  Length                       height = *libPad.getHeight();
  if (isOnSolderMaskTop || isOnSolderMaskBottom) {
    Length         size = qMin(width, height);
    UnsignedLength clearance =
        mBoard.getDesignRules().calcStopMaskClearance(size);
    width += clearance * 2;
    height += clearance * 2;
  } else if (isOnSolderPasteTop || isOnSolderPasteBottom) {
    Length size      = qMin(width, height);
    Length clearance = -mBoard.getDesignRules().calcCreamMaskClearance(size);
    width += clearance * 2;
    height += clearance * 2;
  }

  if ((width <= 0) || (height <= 0)) {
    qWarning() << "Pad with zero size ignored in gerber export:"
               << pad.getLibPadUuid();
    return;
  }

  UnsignedLength uWidth(width);
  UnsignedLength uHeight(height);

  switch (libPad.getShape()) {
    case library::FootprintPad::Shape::ROUND: {
      if (width == height) {
        gen.flashCircle(pad.getPosition(), uWidth, UnsignedLength(0));
      } else {
        gen.flashObround(pad.getPosition(), uWidth, uHeight, rot,
                         UnsignedLength(0));
      }
      break;
    }
    case library::FootprintPad::Shape::RECT: {
      gen.flashRect(pad.getPosition(), uWidth, uHeight, rot, UnsignedLength(0));
      break;
    }
    case library::FootprintPad::Shape::OCTAGON: {
      if (width != height) {
        throw LogicError(
            __FILE__, __LINE__,
            tr("Sorry, non-square octagons are not yet supported."));
      }
      gen.flashRegularPolygon(pad.getPosition(), uWidth, 8, rot,
                              UnsignedLength(0));
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

}  // namespace project
}  // namespace librepcb
