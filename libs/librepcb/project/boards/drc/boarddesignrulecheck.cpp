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
#include "boarddesignrulecheck.h"

#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_airwire.h"
#include "../items/bi_device.h"
#include "../items/bi_footprint.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_hole.h"
#include "../items/bi_netline.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_plane.h"
#include "../items/bi_stroketext.h"
#include "../items/bi_via.h"
#include "boardclipperpathgenerator.h"

#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/common/utils/clipperhelpers.h>
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

BoardDesignRuleCheck::BoardDesignRuleCheck(Board& board, const Options& options,
                                           QObject* parent) noexcept
  : QObject(parent), mBoard(board), mOptions(options), mMessages() {
}

BoardDesignRuleCheck::~BoardDesignRuleCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRuleCheck::execute() {
  emit started();
  emit progressPercent(5);

  mMessages.clear();

  rebuildPlanes(5, 15);
  checkCopperBoardClearances(15, 40);
  checkCopperCopperClearances(40, 70);
  checkMinimumCopperWidth(70, 72);
  checkMinimumPthRestring(72, 74);
  checkMinimumPthDrillDiameter(74, 76);
  checkMinimumNpthDrillDiameter(76, 78);
  checkCourtyardClearances(78, 88);
  checkForMissingConnections(88, 90);

  emit progressStatus(
      tr("Finished with %1 message(s)!", "Count of messages", mMessages.count())
          .arg(mMessages.count()));
  emit progressPercent(100);
  emit finished();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardDesignRuleCheck::rebuildPlanes(int progressStart, int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Rebuild planes..."));
  mBoard.rebuildAllPlanes();
  emit progressPercent(progressEnd);
}

void BoardDesignRuleCheck::checkForMissingConnections(int progressStart,
                                                      int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Check for missing connections..."));

  // No check based on copper paths implemented yet -> return existing airwires
  // instead.
  mBoard.forceAirWiresRebuild();
  foreach (const BI_AirWire* airwire, mBoard.getAirWires()) {
    QString msg = tr("Missing connection: '%1'", "Placeholder is net name")
                      .arg(*airwire->getNetSignal().getName());
    Path location = Path::obround(airwire->getP1(), airwire->getP2(),
                                  PositiveLength(50000));
    addMessage(BoardDesignRuleCheckMessage(msg, location));
  }

  emit progressPercent(progressEnd);
}

void BoardDesignRuleCheck::checkCopperBoardClearances(int progressStart,
                                                      int progressEnd) {
  emit progressStatus(tr("Check board clearances..."));

  qreal progressSpan = progressEnd - progressStart;
  QList<NetSignal*> netsignals =
      mBoard.getProject().getCircuit().getNetSignals().values();
  netsignals.append(nullptr);  // also check unconnected copper objects

  // Board outline
  ClipperLib::Paths outlineRestrictedArea;
  {
    BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
    gen.addBoardOutline();
    outlineRestrictedArea = gen.getPaths();
    ClipperLib::Paths outlinePathsInner = gen.getPaths();
    ClipperHelpers::offset(
        outlinePathsInner,
        *maxArcTolerance() - *mOptions.minCopperBoardClearance,
        maxArcTolerance());
    ClipperHelpers::subtract(outlineRestrictedArea, outlinePathsInner);
  }

  // Holes
  {
    BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
    gen.addHoles(*mOptions.minCopperNpthClearance - *maxArcTolerance());
    ClipperHelpers::unite(outlineRestrictedArea, gen.getPaths());
  }

  auto layers = mBoard.getLayerStack().getAllLayers();
  for (int layerIndex = 0; layerIndex < layers.count(); ++layerIndex) {
    const GraphicsLayer* layer = layers[layerIndex];
    if ((!layer->isCopperLayer()) || (!layer->isEnabled())) {
      continue;
    }
    for (int i = 0; i < netsignals.count(); ++i) {
      std::unique_ptr<ClipperLib::PolyTree> intersections =
          ClipperHelpers::intersect(outlineRestrictedArea,
                                    getCopperPaths(layer, netsignals[i]));
      for (const ClipperLib::Path& path :
           ClipperHelpers::flattenTree(*intersections)) {
        QString name1 = netsignals[i] ? *netsignals[i]->getName() : "";
        QString msg = tr("Clearance (%1): '%2' <-> Board Outline",
                         "Placeholders are layer name + net name")
                          .arg(layer->getNameTr(), name1);
        Path location = ClipperHelpers::convert(path);
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
      qreal progress = progressSpan *
          qreal((layerIndex + 1) * netsignals.count() + i + 1) /
          qreal(layers.count() * netsignals.count());
      emit progressPercent(progressStart + static_cast<int>(progress));
    }
  }
}

void BoardDesignRuleCheck::checkCopperCopperClearances(int progressStart,
                                                       int progressEnd) {
  emit progressStatus(tr("Check copper clearances..."));

  qreal progressSpan = progressEnd - progressStart;
  QList<NetSignal*> netsignals =
      mBoard.getProject().getCircuit().getNetSignals().values();
  netsignals.append(nullptr);  // also check unconnected copper objects

  auto layers = mBoard.getLayerStack().getAllLayers();
  for (int layerIndex = 0; layerIndex < layers.count(); ++layerIndex) {
    const GraphicsLayer* layer = layers[layerIndex];
    if ((!layer->isCopperLayer()) || (!layer->isEnabled())) {
      continue;
    }
    for (int i = 0; i < netsignals.count(); ++i) {
      ClipperLib::Paths paths1 = getCopperPaths(layer, netsignals[i]);
      ClipperHelpers::offset(
          paths1, (*mOptions.minCopperCopperClearance - *maxArcTolerance()) / 2,
          maxArcTolerance());
      for (int k = i + 1; k < netsignals.count(); ++k) {
        ClipperLib::Paths paths2 = getCopperPaths(layer, netsignals[k]);
        ClipperHelpers::offset(
            paths2,
            (*mOptions.minCopperCopperClearance - *maxArcTolerance()) / 2,
            maxArcTolerance());
        std::unique_ptr<ClipperLib::PolyTree> intersections =
            ClipperHelpers::intersect(paths1, paths2);
        for (const ClipperLib::Path& path :
             ClipperHelpers::flattenTree(*intersections)) {
          QString name1 = netsignals[i] ? *netsignals[i]->getName() : "";
          QString name2 = netsignals[k] ? *netsignals[k]->getName() : "";
          QString msg = tr("Clearance (%1): '%2' <-> '%3'",
                           "Placeholders are layer name + net names")
                            .arg(layer->getNameTr(), name1, name2);
          Path location = ClipperHelpers::convert(path);
          addMessage(BoardDesignRuleCheckMessage(msg, location));
        }
      }
      qreal progress = progressSpan *
          qreal((layerIndex + 1) * netsignals.count() + i + 1) /
          qreal(layers.count() * netsignals.count());
      emit progressPercent(progressStart + static_cast<int>(progress));
    }
  }
}

void BoardDesignRuleCheck::checkCourtyardClearances(int progressStart,
                                                    int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Check courtyard clearances..."));

  auto layers = mBoard.getLayerStack().getLayers(
      {GraphicsLayer::sTopCourtyard, GraphicsLayer::sBotCourtyard});
  foreach (const GraphicsLayer* layer, layers) {
    // determine device courtyard areas
    QMap<const BI_Device*, ClipperLib::Paths> deviceCourtyards;
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
      ClipperLib::Paths paths = getDeviceCourtyardPaths(*device, layer);
      ClipperHelpers::offset(paths, mOptions.courtyardOffset,
                             maxArcTolerance());
      deviceCourtyards.insert(device, paths);
    }

    // check clearances
    for (int i = 0; i < deviceCourtyards.count(); ++i) {
      const BI_Device* dev1 = deviceCourtyards.keys()[i];
      Q_ASSERT(dev1);
      const ClipperLib::Paths& paths1 = deviceCourtyards[dev1];
      for (int k = i + 1; k < deviceCourtyards.count(); ++k) {
        const BI_Device* dev2 = deviceCourtyards.keys()[k];
        Q_ASSERT(dev2);
        const ClipperLib::Paths& paths2 = deviceCourtyards[dev2];
        std::unique_ptr<ClipperLib::PolyTree> intersections =
            ClipperHelpers::intersect(paths1, paths2);
        for (const ClipperLib::Path& path :
             ClipperHelpers::flattenTree(*intersections)) {
          QString name1 = *dev1->getComponentInstance().getName();
          QString name2 = *dev2->getComponentInstance().getName();
          QString msg = tr("Clearance (%1): '%2' <-> '%3'",
                           "Placeholders are layer name + component names")
                            .arg(layer->getNameTr(), name1, name2);
          Path location = ClipperHelpers::convert(path);
          addMessage(BoardDesignRuleCheckMessage(msg, location));
        }
      }
    }
  }

  emit progressPercent(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumCopperWidth(int progressStart,
                                                   int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Check minimum copper width..."));

  // stroke texts
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    const GraphicsLayer* layer =
        mBoard.getLayerStack().getLayer(*text->getText().getLayerName());
    if ((!layer) || (!layer->isCopperLayer()) || (!layer->isEnabled())) {
      continue;
    }
    if (text->getText().getStrokeWidth() < mOptions.minCopperWidth) {
      QString msg = tr("Min. copper width (%1) of text: %2",
                       "Placeholders are layer name + width")
                        .arg(layer->getNameTr(),
                             formatLength(*text->getText().getStrokeWidth()));
      QVector<Path> locations;
      foreach (Path path, text->getText().getPaths()) {
        path.rotate(text->getText().getRotation());
        if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
        path.translate(text->getText().getPosition());
        locations += path.toOutlineStrokes(PositiveLength(
            qMax(*text->getText().getStrokeWidth(), Length(50000))));
      }
      addMessage(BoardDesignRuleCheckMessage(msg, locations));
    }
  }

  // planes
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    const GraphicsLayer* layer =
        mBoard.getLayerStack().getLayer(*plane->getLayerName());
    if ((!layer) || (!layer->isCopperLayer()) || (!layer->isEnabled())) {
      continue;
    }
    if (plane->getMinWidth() < mOptions.minCopperWidth) {
      QString msg =
          tr("Min. copper width (%1) of plane: %2",
             "Placeholders are layer name + width")
              .arg(layer->getNameTr(), formatLength(*plane->getMinWidth()));
      QVector<Path> locations =
          plane->getOutline().toClosedPath().toOutlineStrokes(
              PositiveLength(200000));
      addMessage(BoardDesignRuleCheckMessage(msg, locations));
    }
  }

  // devices
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const BI_Footprint& footprint = device->getFootprint();
    foreach (const BI_StrokeText* text, footprint.getStrokeTexts()) {
      // Do *not* mirror layer since it is independent of the device!
      const GraphicsLayer* layer =
          mBoard.getLayerStack().getLayer(*text->getText().getLayerName());
      if ((!layer) || (!layer->isCopperLayer()) || (!layer->isEnabled())) {
        continue;
      }
      if (text->getText().getStrokeWidth() < mOptions.minCopperWidth) {
        QString msg = tr("Min. copper width (%1) of text: %2",
                         "Placeholders are layer name + width")
                          .arg(layer->getNameTr(),
                               formatLength(*text->getText().getStrokeWidth()));
        QVector<Path> locations;
        foreach (Path path, text->getText().getPaths()) {
          path.rotate(text->getText().getRotation());
          if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
          path.translate(text->getText().getPosition());
          locations += path.toOutlineStrokes(PositiveLength(
              qMax(*text->getText().getStrokeWidth(), Length(50000))));
        }
        addMessage(BoardDesignRuleCheckMessage(msg, locations));
      }
    }
  }

  // netlines
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      if ((!netline->getLayer().isCopperLayer()) ||
          (!netline->getLayer().isEnabled())) {
        continue;
      }
      if (netline->getWidth() < mOptions.minCopperWidth) {
        QString msg = tr("Min. copper width (%1) of trace: %2",
                         "Placeholders are layer name + width")
                          .arg(netline->getLayer().getNameTr(),
                               formatLength(*netline->getWidth()));
        Path location = Path::obround(netline->getStartPoint().getPosition(),
                                      netline->getEndPoint().getPosition(),
                                      netline->getWidth());
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
    }
  }

  emit progressPercent(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumPthRestring(int progressStart,
                                                   int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Check minimum PTH restrings..."));

  // vias
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      Length restring = (*via->getSize() - *via->getDrillDiameter() + 1) / 2;
      if (restring < *mOptions.minPthRestring) {
        QString msg = tr("Min. via restring ('%1'): %2",
                         "Placeholders are net name + restring width")
                          .arg(netsegment->getNetNameToDisplay(true),
                               formatLength(restring));
        PositiveLength diameter = via->getDrillDiameter() +
            mOptions.minPthRestring + mOptions.minPthRestring;
        Path location = Path::circle(diameter).translated(via->getPosition());
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
    }
  }

  // pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const BI_Footprint& footprint = device->getFootprint();
    foreach (const BI_FootprintPad* pad, footprint.getPads()) {
      if (pad->getLibPad().getBoardSide() !=
          library::FootprintPad::BoardSide::THT) {
        continue;  // skip SMT pads
      }
      PositiveLength size =
          qMin(pad->getLibPad().getWidth(), pad->getLibPad().getHeight());
      Length restring = (*size - *pad->getLibPad().getDrillDiameter() + 1) / 2;
      if (restring < *mOptions.minPthRestring) {
        QString msg = tr("Min. pad restring ('%1'): %2",
                         "Placeholders are pad name + restring width")
                          .arg(pad->getDisplayText().simplified(),
                               formatLength(restring));
        PositiveLength diameter =
            PositiveLength(pad->getLibPad().getDrillDiameter() + 1) +
            mOptions.minPthRestring + mOptions.minPthRestring;
        Path location = Path::circle(diameter).translated(pad->getPosition());
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
    }
  }

  emit progressPercent(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumPthDrillDiameter(int progressStart,
                                                        int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Check minimum PTH drill diameters..."));

  // vias
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      if (via->getDrillDiameter() < mOptions.minPthDrillDiameter) {
        QString msg = tr("Min. via drill diameter ('%1'): %2",
                         "Placeholders are net name + drill diameter")
                          .arg(netsegment->getNetNameToDisplay(true),
                               formatLength(*via->getDrillDiameter()));
        Path location = Path::circle(via->getDrillDiameter())
                            .translated(via->getPosition());
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
    }
  }

  // pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const BI_Footprint& footprint = device->getFootprint();
    foreach (const BI_FootprintPad* pad, footprint.getPads()) {
      if (pad->getLibPad().getBoardSide() !=
          library::FootprintPad::BoardSide::THT) {
        continue;  // skip SMT pads
      }
      if (pad->getLibPad().getDrillDiameter() < *mOptions.minPthDrillDiameter) {
        QString msg =
            tr("Min. pad drill diameter ('%1'): %2",
               "Placeholders are pad name + drill diameter")
                .arg(pad->getDisplayText().simplified(),
                     formatLength(*pad->getLibPad().getDrillDiameter()));
        PositiveLength diameter(
            qMax(*pad->getLibPad().getDrillDiameter(), Length(50000)));
        Path location = Path::circle(diameter).translated(pad->getPosition());
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
    }
  }

  emit progressPercent(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumNpthDrillDiameter(int progressStart,
                                                         int progressEnd) {
  Q_UNUSED(progressStart);
  emit progressStatus(tr("Check minimum NPTH drill diameters..."));

  QString msgTr = tr("Min. hole diameter: %1", "Placeholder is drill diameter");

  // board holes
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if (hole->getHole().getDiameter() < mOptions.minNpthDrillDiameter) {
      QString msg = msgTr.arg(formatLength(*hole->getHole().getDiameter()));
      Path location = Path::circle(hole->getHole().getDiameter())
                          .translated(hole->getPosition());
      addMessage(BoardDesignRuleCheckMessage(msg, location));
    }
  }

  // package holes
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const BI_Footprint& footprint = device->getFootprint();
    for (const Hole& hole : footprint.getLibFootprint().getHoles()) {
      if (hole.getDiameter() < *mOptions.minNpthDrillDiameter) {
        QString msg = msgTr.arg(formatLength(*hole.getDiameter()));
        Path location =
            Path::circle(hole.getDiameter())
                .translated(footprint.mapToScene(hole.getPosition()));
        addMessage(BoardDesignRuleCheckMessage(msg, location));
      }
    }
  }

  emit progressPercent(progressEnd);
}

const ClipperLib::Paths& BoardDesignRuleCheck::getCopperPaths(
    const GraphicsLayer* layer, const NetSignal* netsignal) {
  if (!mCachedPaths[layer].contains(netsignal)) {
    BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
    gen.addCopper(layer->getName(), netsignal);
    mCachedPaths[layer][netsignal] = gen.getPaths();
  }
  return mCachedPaths[layer][netsignal];
}

ClipperLib::Paths BoardDesignRuleCheck::getDeviceCourtyardPaths(
    const BI_Device& device, const GraphicsLayer* layer) {
  ClipperLib::Paths paths;
  for (const Polygon& polygon : device.getLibFootprint().getPolygons()) {
    QString polygonLayer = *polygon.getLayerName();
    if (device.getIsMirrored()) {
      polygonLayer = GraphicsLayer::getMirroredLayerName(polygonLayer);
    }
    if (polygonLayer != layer->getName()) {
      continue;
    }
    Path path = polygon.getPath();
    path.rotate(device.getRotation());
    if (device.getIsMirrored()) path.mirror(Qt::Horizontal);
    path.translate(device.getPosition());
    ClipperHelpers::unite(paths,
                          ClipperHelpers::convert(path, maxArcTolerance()));
  }
  for (const Circle& circle : device.getLibFootprint().getCircles()) {
    QString circleLayer = *circle.getLayerName();
    if (device.getIsMirrored()) {
      circleLayer = GraphicsLayer::getMirroredLayerName(circleLayer);
    }
    if (circleLayer != layer->getName()) {
      continue;
    }
    Point absolutePos = circle.getCenter();
    absolutePos.rotate(device.getRotation());
    if (device.getIsMirrored()) absolutePos.mirror(Qt::Horizontal);
    absolutePos += device.getPosition();
    ClipperHelpers::unite(
        paths,
        ClipperHelpers::convert(Path::circle(circle.getDiameter()),
                                maxArcTolerance()));
  }
  return paths;
}

void BoardDesignRuleCheck::addMessage(
    const BoardDesignRuleCheckMessage& msg) noexcept {
  mMessages.append(msg);
  emit progressMessage(msg.getMessage());
}

QString BoardDesignRuleCheck::formatLength(const Length& length) const
    noexcept {
  return Toolbox::floatToString(length.toMm(), 6, QLocale()) % "mm";
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
