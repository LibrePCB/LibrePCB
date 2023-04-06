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
#include "boardselectionquery.h"

#include "boardgraphicsscene.h"
#include "graphicsitems/bgi_device.h"
#include "graphicsitems/bgi_footprintpad.h"
#include "graphicsitems/bgi_hole.h"
#include "graphicsitems/bgi_netline.h"
#include "graphicsitems/bgi_netpoint.h"
#include "graphicsitems/bgi_plane.h"
#include "graphicsitems/bgi_polygon.h"
#include "graphicsitems/bgi_stroketext.h"
#include "graphicsitems/bgi_via.h"

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

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardSelectionQuery::BoardSelectionQuery(BoardGraphicsScene& scene,
                                         QObject* parent)
  : QObject(parent), mScene(scene) {
}

BoardSelectionQuery::~BoardSelectionQuery() noexcept {
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

QHash<BI_NetSegment*, BoardSelectionQuery::NetSegmentItems>
    BoardSelectionQuery::getNetSegmentItems() const noexcept {
  QHash<BI_NetSegment*, NetSegmentItems> result;
  foreach (BI_Via* via, mResultVias) {
    result[&via->getNetSegment()].vias.insert(via);
  }
  foreach (BI_NetPoint* netpoint, mResultNetPoints) {
    result[&netpoint->getNetSegment()].netpoints.insert(netpoint);
  }
  foreach (BI_NetLine* netline, mResultNetLines) {
    result[&netline->getNetSegment()].netlines.insert(netline);
  }
  return result;
}

int BoardSelectionQuery::getResultCount() const noexcept {
  return mResultDeviceInstances.count() + mResultNetPoints.count() +
      mResultNetLines.count() + mResultVias.count() + mResultPlanes.count() +
      mResultPolygons.count() + mResultStrokeTexts.count() +
      mResultHoles.count();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardSelectionQuery::addDeviceInstancesOfSelectedFootprints() noexcept {
  for (auto it = mScene.getDevices().begin(); it != mScene.getDevices().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultDeviceInstances.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedVias() noexcept {
  for (auto it = mScene.getVias().begin(); it != mScene.getVias().end(); it++) {
    if (it.value()->isSelected()) {
      mResultVias.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedNetPoints() noexcept {
  for (auto it = mScene.getNetPoints().begin();
       it != mScene.getNetPoints().end(); it++) {
    if (it.value()->isSelected()) {
      mResultNetPoints.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedNetLines() noexcept {
  for (auto it = mScene.getNetLines().begin(); it != mScene.getNetLines().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultNetLines.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedPlanes() noexcept {
  for (auto it = mScene.getPlanes().begin(); it != mScene.getPlanes().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultPlanes.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedPolygons() noexcept {
  for (auto it = mScene.getPolygons().begin(); it != mScene.getPolygons().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultPolygons.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedBoardStrokeTexts() noexcept {
  for (auto it = mScene.getStrokeTexts().begin();
       it != mScene.getStrokeTexts().end(); it++) {
    if ((!it.key()->getDevice()) && it.value()->isSelected()) {
      mResultStrokeTexts.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedFootprintStrokeTexts() noexcept {
  for (auto it = mScene.getStrokeTexts().begin();
       it != mScene.getStrokeTexts().end(); it++) {
    if (it.key()->getDevice() && it.value()->isSelected()) {
      mResultStrokeTexts.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addSelectedHoles() noexcept {
  for (auto it = mScene.getHoles().begin(); it != mScene.getHoles().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultHoles.insert(it.key());
    }
  }
}

void BoardSelectionQuery::addNetPointsOfNetLines(
    bool onlyIfAllNetLinesSelected) noexcept {
  foreach (BI_NetLine* netline, mResultNetLines) {
    BI_NetPoint* p1 = dynamic_cast<BI_NetPoint*>(&netline->getStartPoint());
    BI_NetPoint* p2 = dynamic_cast<BI_NetPoint*>(&netline->getEndPoint());
    if (p1 &&
        ((!onlyIfAllNetLinesSelected) ||
         (mResultNetLines.contains(p1->getNetLines())))) {
      mResultNetPoints.insert(p1);
    }
    if (p2 &&
        ((!onlyIfAllNetLinesSelected) ||
         (mResultNetLines.contains(p2->getNetLines())))) {
      mResultNetPoints.insert(p2);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
