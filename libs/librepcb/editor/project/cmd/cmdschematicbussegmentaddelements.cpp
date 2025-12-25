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
#include "cmdschematicbussegmentaddelements.h"

#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicBusSegmentAddElements::CmdSchematicBusSegmentAddElements(
    SI_BusSegment& segment) noexcept
  : UndoCommand(tr("Add Bus Segment Elements")), mSegment(segment) {
}

CmdSchematicBusSegmentAddElements::
    ~CmdSchematicBusSegmentAddElements() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

SI_BusJunction* CmdSchematicBusSegmentAddElements::addJunction(
    SI_BusJunction& junction) {
  mJunctions.append(&junction);
  return &junction;
}

SI_BusJunction* CmdSchematicBusSegmentAddElements::addJunction(
    const Point& position) {
  SI_BusJunction* netpoint = new SI_BusJunction(mSegment, Uuid::createRandom(),
                                                position);  // can throw
  return addJunction(*netpoint);
}

SI_BusLine* CmdSchematicBusSegmentAddElements::addLine(SI_BusLine& line) {
  mLines.append(&line);
  return &line;
}

SI_BusLine* CmdSchematicBusSegmentAddElements::addLine(SI_BusJunction& a,
                                                       SI_BusJunction& b) {
  SI_BusLine* netline = new SI_BusLine(mSegment, Uuid::createRandom(), a, b,
                                       UnsignedLength(400000));  // can throw
  return addLine(*netline);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicBusSegmentAddElements::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdSchematicBusSegmentAddElements::performUndo() {
  mSegment.removeJunctionsAndLines(mJunctions, mLines);  // can throw
}

void CmdSchematicBusSegmentAddElements::performRedo() {
  mSegment.addJunctionsAndLines(mJunctions, mLines);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
