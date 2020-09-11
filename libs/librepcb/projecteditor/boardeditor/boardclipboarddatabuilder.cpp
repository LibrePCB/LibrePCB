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

#include "boardnetsegmentsplitter.h"

#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/boards/items/bi_polygon.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/netsignal.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardClipboardDataBuilder::BoardClipboardDataBuilder(Board& board) noexcept
  : mBoard(board) {
}

BoardClipboardDataBuilder::~BoardClipboardDataBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<BoardClipboardData> BoardClipboardDataBuilder::generate(
    const Point& cursorPos) const noexcept {
  std::unique_ptr<BoardClipboardData> data(
      new BoardClipboardData(mBoard.getUuid(), cursorPos));

  // Get all selected items
  std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
  query->addDeviceInstancesOfSelectedFootprints();
  query->addSelectedVias();
  query->addSelectedNetLines();
  query->addSelectedPlanes();
  query->addSelectedPolygons();
  query->addSelectedBoardStrokeTexts();
  query->addSelectedHoles();
  query->addNetPointsOfNetLines();

  // Add devices
  foreach (BI_Device* device, query->getDeviceInstances()) {
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
    StrokeTextList strokeTexts;
    foreach (const BI_StrokeText* t, device->getFootprint().getStrokeTexts()) {
      strokeTexts.append(std::make_shared<StrokeText>(t->getText()));
    }
    // Add device
    data->getDevices().append(std::make_shared<BoardClipboardData::Device>(
        device->getComponentInstanceUuid(), device->getLibDevice().getUuid(),
        device->getLibFootprint().getUuid(), device->getPosition(),
        device->getRotation(), device->getIsMirrored(), strokeTexts));
    // Add pad positions
    foreach (const BI_FootprintPad* pad, device->getFootprint().getPads()) {
      data->getPadPositions().insert(
          std::make_pair(device->getComponentInstanceUuid(),
                         pad->getLibPadUuid()),
          pad->getPosition());
    }
  }

  // Add (splitted) net segments including vias, netpoints, and netlines
  QHash<BI_NetSegment*, BoardSelectionQuery::NetSegmentItems> netSegmentItems =
      query->getNetSegmentItems();
  for (auto it = netSegmentItems.constBegin(); it != netSegmentItems.constEnd();
       ++it) {
    BoardNetSegmentSplitter splitter;
    foreach (BI_Device* device, mBoard.getDeviceInstances()) {
      foreach (BI_FootprintPad* pad, device->getFootprint().getPads()) {
        if (pad->getNetSegmentOfLines() == it.key()) {
          if (!query->getDeviceInstances().contains(device)) {
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
      std::shared_ptr<BoardClipboardData::NetSegment> newSegment =
          std::make_shared<BoardClipboardData::NetSegment>(
              it.key()->getNetSignal().getName());
      newSegment->vias      = segment.vias;
      newSegment->junctions = segment.junctions;
      newSegment->traces    = segment.traces;
      data->getNetSegments().append(newSegment);
    }
  }

  // Add planes
  foreach (BI_Plane* plane, query->getPlanes()) {
    std::shared_ptr<BoardClipboardData::Plane> newPlane =
        std::make_shared<BoardClipboardData::Plane>(
            plane->getUuid(), *plane->getLayerName(),
            *plane->getNetSignal().getName(), plane->getOutline(),
            plane->getMinWidth(), plane->getMinClearance(),
            plane->getKeepOrphans(), plane->getPriority(),
            plane->getConnectStyle());
    data->getPlanes().append(newPlane);
  }

  // Add polygons
  foreach (BI_Polygon* polygon, query->getPolygons()) {
    data->getPolygons().append(
        std::make_shared<Polygon>(polygon->getPolygon()));
  }

  // Add stroke texts
  foreach (BI_StrokeText* text, query->getStrokeTexts()) {
    data->getStrokeTexts().append(
        std::make_shared<StrokeText>(text->getText()));
  }

  // Add holes
  foreach (BI_Hole* hole, query->getHoles()) {
    data->getHoles().append(std::make_shared<Hole>(hole->getHole()));
  }

  return data;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
