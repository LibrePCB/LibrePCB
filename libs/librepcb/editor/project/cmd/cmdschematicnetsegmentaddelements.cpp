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
#include "cmdschematicnetsegmentaddelements.h"

#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicNetSegmentAddElements::CmdSchematicNetSegmentAddElements(
    SI_NetSegment& segment) noexcept
  : UndoCommand(tr("Add net segment elements")), mNetSegment(segment) {
}

CmdSchematicNetSegmentAddElements::
    ~CmdSchematicNetSegmentAddElements() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

SI_NetPoint* CmdSchematicNetSegmentAddElements::addNetPoint(
    SI_NetPoint& netpoint) {
  mNetPoints.append(&netpoint);
  return &netpoint;
}

SI_NetPoint* CmdSchematicNetSegmentAddElements::addNetPoint(
    const Point& position) {
  SI_NetPoint* netpoint = new SI_NetPoint(mNetSegment, Uuid::createRandom(),
                                          position);  // can throw
  return addNetPoint(*netpoint);
}

SI_NetLine* CmdSchematicNetSegmentAddElements::addNetLine(SI_NetLine& netline) {
  mNetLines.append(&netline);
  return &netline;
}

SI_NetLine* CmdSchematicNetSegmentAddElements::addNetLine(
    SI_NetLineAnchor& startPoint, SI_NetLineAnchor& endPoint) {
  SI_NetLine* netline =
      new SI_NetLine(mNetSegment, Uuid::createRandom(), startPoint, endPoint,
                     UnsignedLength(158750));  // can throw
  return addNetLine(*netline);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicNetSegmentAddElements::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdSchematicNetSegmentAddElements::performUndo() {
  mNetSegment.removeNetPointsAndNetLines(mNetPoints, mNetLines);  // can throw
}

void CmdSchematicNetSegmentAddElements::performRedo() {
  mNetSegment.addNetPointsAndNetLines(mNetPoints, mNetLines);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
