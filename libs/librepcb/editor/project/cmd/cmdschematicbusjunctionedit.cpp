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
#include "cmdschematicbusjunctionedit.h"

#include <librepcb/core/project/schematic/items/si_busjunction.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicBusJunctionEdit::CmdSchematicBusJunctionEdit(
    SI_BusJunction& junction) noexcept
  : UndoCommand(tr("Edit Bus Junction")),
    mJunction(junction),
    mOldPos(junction.getPosition()),
    mNewPos(mOldPos) {
}

CmdSchematicBusJunctionEdit::~CmdSchematicBusJunctionEdit() noexcept {
  if (!wasEverExecuted()) {
    mJunction.setPosition(mOldPos);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdSchematicBusJunctionEdit::setPosition(const Point& pos,
                                              bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = pos;
  if (immediate) mJunction.setPosition(mNewPos);
}

void CmdSchematicBusJunctionEdit::translate(const Point& deltaPos,
                                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mJunction.setPosition(mNewPos);
}

void CmdSchematicBusJunctionEdit::snapToGrid(const PositiveLength& gridInterval,
                                             bool immediate) noexcept {
  setPosition(mNewPos.mappedToGrid(gridInterval), immediate);
}

void CmdSchematicBusJunctionEdit::rotate(const Angle& angle,
                                         const Point& center,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.rotate(angle, center);
  if (immediate) mJunction.setPosition(mNewPos);
}

void CmdSchematicBusJunctionEdit::mirror(Qt::Orientation orientation,
                                         const Point& center,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.mirror(orientation, center);
  if (immediate) mJunction.setPosition(mNewPos);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicBusJunctionEdit::performExecute() {
  performRedo();  // can throw

  return true;  // TODO: determine if the netpoint was really modified
}

void CmdSchematicBusJunctionEdit::performUndo() {
  mJunction.setPosition(mOldPos);
}

void CmdSchematicBusJunctionEdit::performRedo() {
  mJunction.setPosition(mNewPos);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
