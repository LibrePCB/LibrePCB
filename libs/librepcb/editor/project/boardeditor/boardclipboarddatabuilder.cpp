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
#include "boardclipboarddatabuilder.h"

#include "boardgraphicsscene.h"
#include "boardnetsegmentsplitter.h"
#include "boardselectionquery.h"

#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/circuit/netsignal.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardClipboardDataBuilder::BoardClipboardDataBuilder(
    BoardGraphicsScene& scene) noexcept
  : mScene(scene) {
}

BoardClipboardDataBuilder::~BoardClipboardDataBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<BoardClipboardData> BoardClipboardDataBuilder::generate(
    const Point& cursorPos) const noexcept {
  std::unique_ptr<BoardClipboardData> data(
      new BoardClipboardData(mScene.getBoard().getUuid(), cursorPos));

  // Get all selected items
  BoardSelectionQuery query(mScene, true);
  query.addDeviceInstancesOfSelectedFootprints();
  query.addSelectedVias();
  query.addSelectedNetLines();
  query.addSelectedPlanes();
  query.addSelectedZones();
  query.addSelectedPolygons();
  query.addSelectedBoardStrokeTexts();
  query.addSelectedHoles();
  query.addNetPointsOfNetLines();

  // Add devices
  foreach (BI_Device* device, query.getDeviceInstances()) {
    // Copy library device
    std::unique_ptr<TransactionalDirectory> devDir =
        data->getDirectory("dev/" % device->getLibDevice().getUuid().toStr());
    if (devDir->getFiles().isEmpty()) {
      device->getLibDevice().getDirectory().copyTo(*devDir);
    }
    // Copy library package
    std::unique_ptr<TransactionalDirectory> pkgDir =
        data->getDirectory("pkg/" % device->getLibPackage().getUuid().toStr());
    if (pkgDir->getFiles().isEmpty()) {
      device->getLibPackage().getDirectory().copyTo(*pkgDir);
    }
    // Create list of stroke texts
    QList<BoardStrokeTextData> strokeTexts;
    foreach (const BI_StrokeText* t, device->getStrokeTexts()) {
      strokeTexts.append(t->getData());
    }
    // Add device
    data->getDevices().append(std::make_shared<BoardClipboardData::Device>(
        device->getComponentInstanceUuid(), device->getLibDevice().getUuid(),
        device->getLibFootprint().getUuid(), device->getPosition(),
        device->getRotation(), device->getMirrored(), device->isLocked(),
        device->getAttributes(), strokeTexts));
    // Add pad positions
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      data->getPadPositions().insert(
          std::make_pair(device->getComponentInstanceUuid(),
                         pad->getLibPadUuid()),
          pad->getPosition());
    }
  }

  // Add (splitted) net segments including vias, netpoints, and netlines
  QHash<BI_NetSegment*, BoardSelectionQuery::NetSegmentItems> netSegmentItems =
      query.getNetSegmentItems();
  for (auto it = netSegmentItems.constBegin(); it != netSegmentItems.constEnd();
       ++it) {
    BoardNetSegmentSplitter splitter;
    foreach (BI_Device* device, mScene.getBoard().getDeviceInstances()) {
      foreach (BI_FootprintPad* pad, device->getPads()) {
        if (pad->getNetSegmentOfLines() == it.key()) {
          if (!query.getDeviceInstances().contains(device)) {
            // Pad is currently connected to this net segment, but will not be
            // copied. Thus it needs to be replaced by junctions.
            splitter.replaceFootprintPadByJunctions(pad->toTraceAnchor(),
                                                    pad->getPosition());
          }
        }
      }
    }
    foreach (BI_Via* via, it.key()->getVias()) {
      bool replaceByJunctions = !it.value().vias.contains(via);
      splitter.addVia(via->getVia(), replaceByJunctions);
    }
    foreach (BI_NetPoint* netpoint, it.value().netpoints) {
      splitter.addJunction(netpoint->getJunction());
    }
    foreach (BI_NetLine* netline, it.value().netlines) {
      splitter.addTrace(netline->getTrace());
    }

    foreach (const BoardNetSegmentSplitter::Segment& segment,
             splitter.split()) {
      tl::optional<CircuitIdentifier> netName;
      if (const NetSignal* netsignal = it.key()->getNetSignal()) {
        netName = netsignal->getName();
      }
      std::shared_ptr<BoardClipboardData::NetSegment> newSegment =
          std::make_shared<BoardClipboardData::NetSegment>(netName);
      newSegment->vias = segment.vias;
      newSegment->junctions = segment.junctions;
      newSegment->traces = segment.traces;
      data->getNetSegments().append(newSegment);
    }
  }

  // Add planes
  foreach (BI_Plane* plane, query.getPlanes()) {
    std::shared_ptr<BoardClipboardData::Plane> newPlane =
        std::make_shared<BoardClipboardData::Plane>(
            plane->getUuid(), plane->getLayer(),
            plane->getNetSignal()
                ? tl::make_optional(plane->getNetSignal()->getName())
                : tl::nullopt,
            plane->getOutline(), plane->getMinWidth(), plane->getMinClearance(),
            plane->getKeepIslands(), plane->getPriority(),
            plane->getConnectStyle(), plane->getThermalGap(),
            plane->getThermalSpokeWidth(), plane->isLocked());
    data->getPlanes().append(newPlane);
  }

  // Add zones
  foreach (BI_Zone* zone, query.getZones()) {
    data->getZones().append(zone->getData());
  }

  // Add polygons
  foreach (BI_Polygon* polygon, query.getPolygons()) {
    data->getPolygons().append(polygon->getData());
  }

  // Add stroke texts
  foreach (BI_StrokeText* t, query.getStrokeTexts()) {
    data->getStrokeTexts().append(t->getData());
  }

  // Add holes
  foreach (BI_Hole* hole, query.getHoles()) {
    data->getHoles().append(hole->getData());
  }

  return data;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
