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
#include "cmdboardholeedit.h"

#include <librepcb/core/project/board/items/bi_hole.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardHoleEdit::CmdBoardHoleEdit(BI_Hole& hole) noexcept
  : UndoCommand(tr("Edit hole")),
    mHole(hole),
    mOldData(hole.getData()),
    mNewData(mOldData) {
}

CmdBoardHoleEdit::~CmdBoardHoleEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardHoleEdit::setPath(const NonEmptyPath& path,
                               bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setPath(path) && immediate) {
    mHole.setPath(path);
  }
}

void CmdBoardHoleEdit::translate(const Point& deltaPos,
                                 bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPath(NonEmptyPath(mNewData.getPath()->translated(deltaPos)), immediate);
}

void CmdBoardHoleEdit::snapToGrid(const PositiveLength& gridInterval,
                                  bool immediate) noexcept {
  const Point p0 = mNewData.getPath()->getVertices().first().getPos();
  const Point p1 = p0.mappedToGrid(gridInterval);
  setPath(NonEmptyPath(mNewData.getPath()->translated(p1 - p0)), immediate);
}

void CmdBoardHoleEdit::rotate(const Angle& angle, const Point& center,
                              bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPath(NonEmptyPath(mNewData.getPath()->rotated(angle, center)), immediate);
}

void CmdBoardHoleEdit::mirror(Qt::Orientation orientation, const Point& center,
                              bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPath(NonEmptyPath(mNewData.getPath()->mirrored(orientation, center)),
          immediate);
}

void CmdBoardHoleEdit::setDiameter(const PositiveLength& diameter,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setDiameter(diameter) && immediate) {
    mHole.setDiameter(diameter);
  }
}

void CmdBoardHoleEdit::setStopMaskConfig(const MaskConfig& config) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewData.setStopMaskConfig(config);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardHoleEdit::performExecute() {
  performRedo();  // can throw
  return (mNewData != mOldData);
}

void CmdBoardHoleEdit::performUndo() {
  mHole.setPath(mOldData.getPath());
  mHole.setDiameter(mOldData.getDiameter());
  mHole.setStopMaskConfig(mOldData.getStopMaskConfig());
}

void CmdBoardHoleEdit::performRedo() {
  mHole.setPath(mNewData.getPath());
  mHole.setDiameter(mNewData.getDiameter());
  mHole.setStopMaskConfig(mNewData.getStopMaskConfig());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
