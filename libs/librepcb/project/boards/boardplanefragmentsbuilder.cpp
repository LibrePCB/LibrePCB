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
#include "boardplanefragmentsbuilder.h"
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/utils/clipperhelpers.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintpad.h>
#include "items/bi_plane.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_netsegment.h"
#include "items/bi_via.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include "items/bi_polygon.h"
#include "items/bi_hole.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardPlaneFragmentsBuilder::BoardPlaneFragmentsBuilder(BI_Plane& plane) noexcept :
    mPlane(plane)
{
}

BoardPlaneFragmentsBuilder::~BoardPlaneFragmentsBuilder() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QVector<Path> BoardPlaneFragmentsBuilder::buildFragments() noexcept
{
    try {
        mResult.clear();
        addPlaneOutline();
        clipToBoardOutline();
        subtractOtherObjects();
        ensureMinimumWidth();
        flattenResult();
        if (!mPlane.getKeepOrphans()) {
            removeOrphans();
        }
        return ClipperHelpers::convert(mResult);
    } catch (const Exception& e) {
        qCritical() << "Failed to build plane fragments! Leave plane empty...";
        qCritical() << "Inner error message:" << e.getMsg();
        return QVector<Path>();
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardPlaneFragmentsBuilder::addPlaneOutline()
{
    mResult.push_back(ClipperHelpers::convert(mPlane.getOutline(), maxArcTolerance()));
}

void BoardPlaneFragmentsBuilder::clipToBoardOutline()
{
    // determine board area
    ClipperLib::Paths boardArea;
    ClipperLib::Clipper boardAreaClipper;
    foreach (const BI_Polygon* polygon, mPlane.getBoard().getPolygons()) {
        if (polygon->getPolygon().getLayerName() == GraphicsLayer::sBoardOutlines) {
            ClipperLib::Path path = ClipperHelpers::convert(polygon->getPolygon().getPath(),
                                                            maxArcTolerance());
            boardAreaClipper.AddPath(path, ClipperLib::ptSubject, true);
        }
    }
    boardAreaClipper.Execute(ClipperLib::ctXor, boardArea, ClipperLib::pftEvenOdd,
                             ClipperLib::pftEvenOdd);

    // perform clearance offset
    ClipperHelpers::offset(boardArea, -mPlane.getMinClearance(), maxArcTolerance()); // can throw

    // if we have no board area, abort here
    if (boardArea.empty()) return;

    // clip result to board area
    ClipperLib::Clipper clip;
    clip.AddPaths(mResult, ClipperLib::ptSubject, true);
    clip.AddPaths(boardArea, ClipperLib::ptClip, true);
    clip.Execute(ClipperLib::ctIntersection, mResult, ClipperLib::pftNonZero,
                 ClipperLib::pftNonZero);
}

void BoardPlaneFragmentsBuilder::subtractOtherObjects()
{
    ClipperLib::Clipper c;
    c.AddPaths(mResult, ClipperLib::ptSubject, true);

    // subtract other planes
    foreach (const BI_Plane* plane, mPlane.getBoard().getPlanes()) {
        if (plane == &mPlane) continue;
        if (*plane < mPlane) continue; // ignore planes with lower priority
        if (plane->getLayerName() != mPlane.getLayerName()) continue;
        if (&plane->getNetSignal() == &mPlane.getNetSignal()) continue;
        ClipperLib::Paths paths = ClipperHelpers::convert(plane->getFragments(),
                                                          maxArcTolerance());
        ClipperHelpers::offset(paths, *mPlane.getMinClearance(), maxArcTolerance()); // can throw
        c.AddPaths(paths, ClipperLib::ptClip, true);
    }

    // subtract holes and pads from devices
    foreach (const BI_Device* device, mPlane.getBoard().getDeviceInstances()) {
        for (const Hole& hole : device->getFootprint().getLibFootprint().getHoles()) {
            Point pos = device->getFootprint().mapToScene(hole.getPosition());
            PositiveLength dia(hole.getDiameter() + mPlane.getMinClearance() * 2);
            Path path = Path::circle(dia).translated(pos);
            c.AddPath(ClipperHelpers::convert(path, maxArcTolerance()),
                      ClipperLib::ptClip, true);
        }
        foreach (const BI_FootprintPad* pad, device->getFootprint().getPads()) {
            if (!pad->isOnLayer(mPlane.getLayerName())) continue;
            if (pad->getCompSigInstNetSignal() == &mPlane.getNetSignal()) {
                ClipperLib::Path path = ClipperHelpers::convert(pad->getSceneOutline(),
                                                                maxArcTolerance());
                mConnectedNetSignalAreas.push_back(path);
            }
            c.AddPath(createPadCutOut(*pad), ClipperLib::ptClip, true);
        }
    }

    // subtract board holes
    for (const BI_Hole* hole : mPlane.getBoard().getHoles()) {
        PositiveLength dia(hole->getHole().getDiameter() + mPlane.getMinClearance() * 2);
        Path path = Path::circle(dia).translated(hole->getHole().getPosition());
        c.AddPath(ClipperHelpers::convert(path, maxArcTolerance()),
                  ClipperLib::ptClip, true);
    }

    // subtract net segment items
    foreach (const BI_NetSegment* netsegment, mPlane.getBoard().getNetSegments()) {

        // subtract vias
        foreach (const BI_Via* via, netsegment->getVias()) {
            if (&netsegment->getNetSignal() == &mPlane.getNetSignal()) {
                ClipperLib::Path path = ClipperHelpers::convert(via->getSceneOutline(),
                                                                maxArcTolerance());
                mConnectedNetSignalAreas.push_back(path);
            }
            c.AddPath(createViaCutOut(*via), ClipperLib::ptClip, true);
        }

        // subtract netlines
        foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
            if (netline->getLayer().getName() != mPlane.getLayerName()) continue;
            if (&netsegment->getNetSignal() == &mPlane.getNetSignal()) {
                ClipperLib::Path path = ClipperHelpers::convert(netline->getSceneOutline(),
                                                                maxArcTolerance());
                mConnectedNetSignalAreas.push_back(path);
            } else {
                ClipperLib::Path path = ClipperHelpers::convert(
                    netline->getSceneOutline(*mPlane.getMinClearance()), maxArcTolerance());
                c.AddPath(path, ClipperLib::ptClip, true);
            }
        }
    }

    c.Execute(ClipperLib::ctDifference, mResult, ClipperLib::pftEvenOdd,
              ClipperLib::pftNonZero);
}

void BoardPlaneFragmentsBuilder::ensureMinimumWidth()
{
    Length delta = mPlane.getMinWidth() / 2;
    ClipperHelpers::offset(mResult, -delta, maxArcTolerance()); // can throw
    ClipperHelpers::offset(mResult, delta, maxArcTolerance()); // can throw
}

void BoardPlaneFragmentsBuilder::flattenResult()
{
    // convert paths to tree
    ClipperLib::PolyTree tree;
    ClipperLib::Clipper c;
    c.AddPaths(mResult, ClipperLib::ptSubject, true);
    c.Execute(ClipperLib::ctXor, tree, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

    // convert tree to simple paths with cut-ins
    mResult = ClipperHelpers::flattenTree(tree); // can throw
}

void BoardPlaneFragmentsBuilder::removeOrphans()
{
    mResult.erase(std::remove_if(mResult.begin(), mResult.end(),
        [this](const ClipperLib::Path& p){
            ClipperLib::Paths intersections;
            ClipperLib::Clipper c;
            c.AddPaths(mConnectedNetSignalAreas, ClipperLib::ptSubject, true);
            c.AddPath(p, ClipperLib::ptClip, true);
            c.Execute(ClipperLib::ctIntersection, intersections, ClipperLib::pftNonZero,
                      ClipperLib::pftNonZero);
            return intersections.empty();
        }),
        mResult.end());
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

ClipperLib::Path BoardPlaneFragmentsBuilder::createPadCutOut(const BI_FootprintPad& pad) const noexcept
{
    bool differentNetSignal = (pad.getCompSigInstNetSignal() != &mPlane.getNetSignal());
    if ((mPlane.getConnectStyle() == BI_Plane::ConnectStyle::None) || differentNetSignal) {
        return ClipperHelpers::convert(pad.getSceneOutline(*mPlane.getMinClearance()), maxArcTolerance());
    } else {
        return ClipperLib::Path();
    }
}

ClipperLib::Path BoardPlaneFragmentsBuilder::createViaCutOut(const BI_Via& via) const noexcept
{
    bool differentNetSignal = (&via.getNetSignalOfNetSegment() != &mPlane.getNetSignal());
    if ((mPlane.getConnectStyle() == BI_Plane::ConnectStyle::None) || differentNetSignal) {
        return ClipperHelpers::convert(via.getSceneOutline(*mPlane.getMinClearance()), maxArcTolerance());
    } else {
        return ClipperLib::Path();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
