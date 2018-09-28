/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "board.h"
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
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardSelectionQuery::BoardSelectionQuery(
    const QMap<Uuid, BI_Device*>& deviceInstances,
    const QList<BI_NetSegment*>& netsegments, const QList<BI_Plane*>& planes,
    const QList<BI_Polygon*>&    polygons,
    const QList<BI_StrokeText*>& strokeTexts, const QList<BI_Hole*>& holes,
    QObject* parent)
  : QObject(parent),
    mDevices(deviceInstances),
    mNetSegments(netsegments),
    mPlanes(planes),
    mPolygons(polygons),
    mStrokeTexts(strokeTexts),
    mHoles(holes) {
}

BoardSelectionQuery::~BoardSelectionQuery() noexcept {
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

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
  foreach (BI_Device* device, mDevices) {
    if (device->getFootprint().isSelected()) {
      mResultDeviceInstances.insert(device);
    }
  }
}

void BoardSelectionQuery::addSelectedVias() noexcept {
  foreach (BI_NetSegment* netsegment, mNetSegments) {
    foreach (BI_Via* via, netsegment->getVias()) {
      if (via->isSelected()) {
        mResultVias.insert(via);
      }
    }
  }
}

void BoardSelectionQuery::addSelectedNetPoints() noexcept {
  foreach (BI_NetSegment* netsegment, mNetSegments) {
    foreach (BI_NetPoint* netpoint, netsegment->getNetPoints()) {
      if (netpoint->isSelected()) {
        mResultNetPoints.insert(netpoint);
      }
    }
  }
}

void BoardSelectionQuery::addSelectedNetLines() noexcept {
  foreach (BI_NetSegment* netsegment, mNetSegments) {
    foreach (BI_NetLine* netline, netsegment->getNetLines()) {
      if (netline->isSelected()) {
        mResultNetLines.insert(netline);
      }
    }
  }
}

void BoardSelectionQuery::addSelectedPlanes() noexcept {
  foreach (BI_Plane* plane, mPlanes) {
    if (plane->isSelected()) {
      mResultPlanes.insert(plane);
    }
  }
}

void BoardSelectionQuery::addSelectedPolygons() noexcept {
  foreach (BI_Polygon* polygon, mPolygons) {
    if (polygon->isSelected()) {
      mResultPolygons.insert(polygon);
    }
  }
}

void BoardSelectionQuery::addSelectedBoardStrokeTexts() noexcept {
  foreach (BI_StrokeText* text, mStrokeTexts) {
    if (text->isSelected()) {
      mResultStrokeTexts.insert(text);
    }
  }
}

void BoardSelectionQuery::addSelectedFootprintStrokeTexts() noexcept {
  foreach (BI_Device* device, mDevices) {
    foreach (BI_StrokeText* text, device->getFootprint().getStrokeTexts()) {
      if (text->isSelected()) {
        mResultStrokeTexts.insert(text);
      }
    }
  }
}

void BoardSelectionQuery::addSelectedHoles() noexcept {
  foreach (BI_Hole* hole, mHoles) {
    if (hole->isSelected()) {
      mResultHoles.insert(hole);
    }
  }
}

void BoardSelectionQuery::addNetPointsOfNetLines() noexcept {
  foreach (BI_NetLine* netline, mResultNetLines) {
    BI_NetPoint* p1 = dynamic_cast<BI_NetPoint*>(&netline->getStartPoint());
    BI_NetPoint* p2 = dynamic_cast<BI_NetPoint*>(&netline->getEndPoint());
    if (p1) mResultNetPoints.insert(p1);
    if (p2) mResultNetPoints.insert(p2);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
