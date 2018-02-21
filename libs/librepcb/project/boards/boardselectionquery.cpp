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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "boardselectionquery.h"
#include "board.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_netsegment.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include "items/bi_via.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardSelectionQuery::BoardSelectionQuery(const QMap<Uuid, BI_Device*>& deviceInstances,
                                         const QList<BI_NetSegment*>& netsegments,
                                         const QList<BI_Plane*>& planes,
                                         const QList<BI_Polygon*>& polygons,
                                         QObject* parent) :
    QObject(parent), mDevices(deviceInstances),
    mNetSegments(netsegments), mPlanes(planes), mPolygons(polygons)
{
}

BoardSelectionQuery::~BoardSelectionQuery() noexcept
{
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

int BoardSelectionQuery::getResultCount() const noexcept
{
    return  //mResultDeviceInstances.count() +
            mResultFootprints.count() +
            mResultNetPoints.count() +
            mResultNetLines.count() +
            mResultVias.count() +
            mResultPlanes.count() +
            mResultPolygons.count();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardSelectionQuery::addSelectedFootprints() noexcept
{
    foreach (BI_Device* device, mDevices) {
        if (device->getFootprint().isSelected()) {
            mResultFootprints.insert(&device->getFootprint());
        }
    }
}

void BoardSelectionQuery::addSelectedVias() noexcept
{
    foreach (BI_NetSegment* netsegment, mNetSegments) {
        foreach (BI_Via* via, netsegment->getVias()) {
            if (via->isSelected()) {
                mResultVias.insert(via);
            }
        }
    }
}

void BoardSelectionQuery::addSelectedNetPoints(NetPointFilters f) noexcept
{
    foreach (BI_NetSegment* netsegment, mNetSegments) {
        foreach (BI_NetPoint* netpoint, netsegment->getNetPoints()) {
            if (netpoint->isSelected() && doesNetPointMatchFilter(*netpoint, f)) {
                mResultNetPoints.insert(netpoint);
            }
        }
    }
}

void BoardSelectionQuery::addSelectedNetLines(NetLineFilters f) noexcept
{
    foreach (BI_NetSegment* netsegment, mNetSegments) {
        foreach (BI_NetLine* netline, netsegment->getNetLines()) {
            if (netline->isSelected() && doesNetLineMatchFilter(*netline, f)) {
                mResultNetLines.insert(netline);
            }
        }
    }
}

void BoardSelectionQuery::addNetPointsOfNetLines(NetLineFilters lf, NetPointFilters pf) noexcept
{
    foreach (BI_NetLine* netline, mResultNetLines) {
        if (doesNetLineMatchFilter(*netline, lf)) {
            if (doesNetPointMatchFilter(netline->getStartPoint(), pf)) {
                mResultNetPoints.insert(&netline->getStartPoint());
            }
            if (doesNetPointMatchFilter(netline->getEndPoint(), pf)) {
                mResultNetPoints.insert(&netline->getEndPoint());
            }
        }
    }
}

void BoardSelectionQuery::addSelectedPlanes() noexcept
{
    foreach (BI_Plane* plane, mPlanes) {
        if (plane->isSelected()) {
            mResultPlanes.insert(plane);
        }
    }
}

void BoardSelectionQuery::addSelectedPolygons() noexcept
{
    foreach (BI_Polygon* polygon, mPolygons) {
        if (polygon->isSelected()) {
            mResultPolygons.insert(polygon);
        }
    }
}

bool BoardSelectionQuery::doesNetPointMatchFilter(const BI_NetPoint& p, NetPointFilters f) noexcept
{
    if (f.testFlag(NetPointFilter::Floating) && (!p.isAttached())) return true;
    if (f.testFlag(NetPointFilter::Attached) && (p.isAttached())) return true;
    if (f.testFlag(NetPointFilter::AllConnectedLinesSelected)) {
        bool allLinesSelected = true;
        foreach (const BI_NetLine* netline, p.getLines()) {
            if (!netline->isSelected()) {
                allLinesSelected = false;
                break;
            }
        }
        if (allLinesSelected) return true;
    }
    return false;
}

bool BoardSelectionQuery::doesNetLineMatchFilter(const BI_NetLine& l, NetLineFilters f) noexcept
{
    if (f.testFlag(NetLineFilter::Floating) && (!l.isAttached())) return true;
    if (f.testFlag(NetLineFilter::Attached) && (l.isAttached())) return true;
    return false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
