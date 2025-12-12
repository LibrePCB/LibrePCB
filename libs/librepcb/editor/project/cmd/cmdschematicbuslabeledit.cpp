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
#include "cmdschematicbuslabeledit.h"

#include <librepcb/core/project/schematic/items/si_buslabel.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicBusLabelEdit::CmdSchematicBusLabelEdit(SI_BusLabel& label) noexcept
  : UndoCommand(tr("Edit Bus Label")),
    mLabel(label),
    mOldMirrored(label.getMirrored()),
    mNewMirrored(mOldMirrored),
    mOldPos(label.getPosition()),
    mNewPos(mOldPos),
    mOldRotation(label.getRotation()),
    mNewRotation(mOldRotation) {
}

CmdSchematicBusLabelEdit::~CmdSchematicBusLabelEdit() noexcept {
  if (!wasEverExecuted()) {
    // revert temporary changes
    mLabel.setMirrored(mOldMirrored);
    mLabel.setPosition(mOldPos);
    mLabel.setRotation(mOldRotation);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdSchematicBusLabelEdit::setPosition(const Point& position,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = position;
  if (immediate) mLabel.setPosition(mNewPos);
}

void CmdSchematicBusLabelEdit::translate(const Point& deltaPos,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mLabel.setPosition(mNewPos);
}

void CmdSchematicBusLabelEdit::snapToGrid(const PositiveLength& gridInterval,
                                          bool immediate) noexcept {
  setPosition(mNewPos.mappedToGrid(gridInterval), immediate);
}

void CmdSchematicBusLabelEdit::setRotation(const Angle& angle,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mLabel.setRotation(mNewRotation);
}

void CmdSchematicBusLabelEdit::rotate(const Angle& angle, const Point& center,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.rotate(angle, center);
  mNewRotation += angle;
  if (immediate) {
    mLabel.setPosition(mNewPos);
    mLabel.setRotation(mNewRotation);
  }
}

void CmdSchematicBusLabelEdit::mirror(Qt::Orientation orientation,
                                      const Point& center,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMirrored = !mNewMirrored;
  mNewPos.mirror(orientation, center);
  Angle rotation = mNewRotation;
  if (orientation == Qt::Vertical) {
    rotation += Angle::deg180();
    rotate(Angle::deg180(), mNewPos, false);
  }
  rotation.mapTo0_360deg();
  if ((rotation == Angle::deg90()) || (rotation == Angle::deg270())) {
    rotate(Angle::deg180(), mNewPos, false);
  }
  if (immediate) {
    mLabel.setPosition(mNewPos);
    mLabel.setRotation(mNewRotation);
    mLabel.setMirrored(mNewMirrored);
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicBusLabelEdit::performExecute() {
  performRedo();  // can throw

  return true;  // TODO: determine if the netlabel was really modified
}

void CmdSchematicBusLabelEdit::performUndo() {
  mLabel.setPosition(mOldPos);
  mLabel.setRotation(mOldRotation);
  mLabel.setMirrored(mOldMirrored);
}

void CmdSchematicBusLabelEdit::performRedo() {
  mLabel.setPosition(mNewPos);
  mLabel.setRotation(mNewRotation);
  mLabel.setMirrored(mNewMirrored);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
