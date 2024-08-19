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
#include "cmdholeedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdHoleEdit::CmdHoleEdit(Hole& hole) noexcept
  : UndoCommand(tr("Edit hole")),
    mHole(hole),
    mOldPath(hole.getPath()),
    mNewPath(mOldPath),
    mOldDiameter(hole.getDiameter()),
    mNewDiameter(mOldDiameter),
    mOldStopMaskConfig(hole.getStopMaskConfig()),
    mNewStopMaskConfig(mOldStopMaskConfig) {
}

CmdHoleEdit::~CmdHoleEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdHoleEdit::setPath(const NonEmptyPath& path, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPath = path;
  if (immediate) mHole.setPath(mNewPath);
}

void CmdHoleEdit::setPositionOfFirstVertex(const Point& pos,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  translate(pos - mNewPath->getVertices().first().getPos(), immediate);
}

void CmdHoleEdit::translate(const Point& deltaPos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPath(NonEmptyPath(mNewPath->translated(deltaPos)), immediate);
}

void CmdHoleEdit::snapToGrid(const PositiveLength& gridInterval,
                             bool immediate) noexcept {
  const Point p0 = mNewPath->getVertices().first().getPos();
  const Point p1 = p0.mappedToGrid(gridInterval);
  setPath(NonEmptyPath(mNewPath->translated(p1 - p0)), immediate);
}

void CmdHoleEdit::rotate(const Angle& angle, const Point& center,
                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPath(NonEmptyPath(mNewPath->rotated(angle, center)), immediate);
}

void CmdHoleEdit::mirror(Qt::Orientation orientation, const Point& center,
                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPath(NonEmptyPath(mNewPath->mirrored(orientation, center)), immediate);
}

void CmdHoleEdit::setDiameter(const PositiveLength& diameter,
                              bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDiameter = diameter;
  if (immediate) mHole.setDiameter(mNewDiameter);
}

void CmdHoleEdit::setStopMaskConfig(const MaskConfig& config) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewStopMaskConfig = config;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdHoleEdit::performExecute() {
  performRedo();  // can throw

  if (mNewPath != mOldPath) return true;
  if (mNewDiameter != mOldDiameter) return true;
  if (mNewStopMaskConfig != mOldStopMaskConfig) return true;
  return false;
}

void CmdHoleEdit::performUndo() {
  mHole.setPath(mOldPath);
  mHole.setDiameter(mOldDiameter);
  mHole.setStopMaskConfig(mOldStopMaskConfig);
}

void CmdHoleEdit::performRedo() {
  mHole.setPath(mNewPath);
  mHole.setDiameter(mNewDiameter);
  mHole.setStopMaskConfig(mNewStopMaskConfig);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
