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
#include "cmdboardnetsegmentaddelements.h"
#include "../items/bi_netpoint.h"
#include "../items/bi_netline.h"
#include "../items/bi_netsegment.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardNetSegmentAddElements::CmdBoardNetSegmentAddElements(BI_NetSegment& segment) noexcept :
    UndoCommand(tr("Add net segment elements")),
    mNetSegment(segment)
{
}

CmdBoardNetSegmentAddElements::~CmdBoardNetSegmentAddElements() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BI_Via* CmdBoardNetSegmentAddElements::addVia(BI_Via& via)
{
    mVias.append(&via);
    return &via;
}

BI_Via* CmdBoardNetSegmentAddElements::addVia(const Point& position, BI_Via::Shape shape,
                                              const PositiveLength& size,
                                              const PositiveLength& drillDiameter)
{
    BI_Via* via = new BI_Via(mNetSegment, position, shape, size, drillDiameter);
    return addVia(*via);
}

BI_NetPoint* CmdBoardNetSegmentAddElements::addNetPoint(BI_NetPoint& netpoint)
{
    mNetPoints.append(&netpoint);
    return &netpoint;
}

BI_NetPoint* CmdBoardNetSegmentAddElements::addNetPoint(GraphicsLayer& layer,
                                                        const Point& position)
{
    BI_NetPoint* netpoint = new BI_NetPoint(mNetSegment, layer, position); // can throw
    return addNetPoint(*netpoint);
}

BI_NetPoint* CmdBoardNetSegmentAddElements::addNetPoint(GraphicsLayer& layer,
                                                        BI_FootprintPad& pad)
{
    BI_NetPoint* netpoint = new BI_NetPoint(mNetSegment, layer, pad); // can throw
    return addNetPoint(*netpoint);
}

BI_NetPoint* CmdBoardNetSegmentAddElements::addNetPoint(GraphicsLayer& layer, BI_Via& via)
{
    BI_NetPoint* netpoint = new BI_NetPoint(mNetSegment, layer, via); // can throw
    return addNetPoint(*netpoint);
}

BI_NetLine* CmdBoardNetSegmentAddElements::addNetLine(BI_NetLine& netline)
{
    mNetLines.append(&netline);
    return &netline;
}

BI_NetLine* CmdBoardNetSegmentAddElements::addNetLine(BI_NetPoint& startPoint,
                                                      BI_NetPoint& endPoint,
                                                      const PositiveLength& width)
{
    BI_NetLine* netline = new BI_NetLine(startPoint, endPoint, width); // can throw
    return addNetLine(*netline);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardNetSegmentAddElements::performExecute()
{
    performRedo(); // can throw

    return true;
}

void CmdBoardNetSegmentAddElements::performUndo()
{
    mNetSegment.removeElements(mVias, mNetPoints, mNetLines); // can throw
}

void CmdBoardNetSegmentAddElements::performRedo()
{
    mNetSegment.addElements(mVias, mNetPoints, mNetLines); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
