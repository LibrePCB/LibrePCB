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
#include "cmdschematicnetlabeledit.h"

#include <librepcb/core/project/schematic/items/si_netlabel.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicNetLabelEdit::CmdSchematicNetLabelEdit(
    SI_NetLabel& netlabel) noexcept
  : UndoCommand(tr("Edit netlabel")),
    mNetLabel(netlabel),
    mOldMirrored(netlabel.getMirrored()),
    mNewMirrored(mOldMirrored),
    mOldPos(netlabel.getPosition()),
    mNewPos(mOldPos),
    mOldRotation(netlabel.getRotation()),
    mNewRotation(mOldRotation) {
}

CmdSchematicNetLabelEdit::~CmdSchematicNetLabelEdit() noexcept {
  if (!wasEverExecuted()) {
    // revert temporary changes
    mNetLabel.setMirrored(mOldMirrored);
    mNetLabel.setPosition(mOldPos);
    mNetLabel.setRotation(mOldRotation);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdSchematicNetLabelEdit::setPosition(const Point& position,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = position;
  if (immediate) mNetLabel.setPosition(mNewPos);
}

void CmdSchematicNetLabelEdit::translate(const Point& deltaPos,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mNetLabel.setPosition(mNewPos);
}

void CmdSchematicNetLabelEdit::setRotation(const Angle& angle,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mNetLabel.setRotation(mNewRotation);
}

void CmdSchematicNetLabelEdit::rotate(const Angle& angle, const Point& center,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.rotate(angle, center);
  mNewRotation += angle;
  if (immediate) {
    mNetLabel.setPosition(mNewPos);
    mNetLabel.setRotation(mNewRotation);
  }
}

void CmdSchematicNetLabelEdit::mirror(bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMirrored = !mNewMirrored;
  if (immediate) mNetLabel.setMirrored(mNewMirrored);
}

void CmdSchematicNetLabelEdit::mirror(Qt::Orientation orientation,
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
    mNetLabel.setPosition(mNewPos);
    mNetLabel.setRotation(mNewRotation);
    mNetLabel.setMirrored(mNewMirrored);
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicNetLabelEdit::performExecute() {
  performRedo();  // can throw

  return true;  // TODO: determine if the netlabel was really modified
}

void CmdSchematicNetLabelEdit::performUndo() {
  mNetLabel.setPosition(mOldPos);
  mNetLabel.setRotation(mOldRotation);
  mNetLabel.setMirrored(mOldMirrored);
}

void CmdSchematicNetLabelEdit::performRedo() {
  mNetLabel.setPosition(mNewPos);
  mNetLabel.setRotation(mNewRotation);
  mNetLabel.setMirrored(mNewMirrored);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
