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
#include "cmdboardpolygonedit.h"

#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardPolygonEdit::CmdBoardPolygonEdit(BI_Polygon& polygon) noexcept
  : UndoCommand(tr("Edit polygon")),
    mPolygon(polygon),
    mOldData(polygon.getData()),
    mNewData(mOldData) {
}

CmdBoardPolygonEdit::~CmdBoardPolygonEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardPolygonEdit::setLayer(const Layer& layer,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setLayer(layer) && immediate) {
    mPolygon.setLayer(layer);
  }
}

void CmdBoardPolygonEdit::setLineWidth(const UnsignedLength& width,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setLineWidth(width) && immediate) {
    mPolygon.setLineWidth(width);
  }
}

void CmdBoardPolygonEdit::setIsFilled(bool filled, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setIsFilled(filled) && immediate) {
    mPolygon.setIsFilled(filled);
  }
}

void CmdBoardPolygonEdit::setIsGrabArea(bool grabArea,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setIsGrabArea(grabArea) && immediate) {
    mPolygon.setIsGrabArea(grabArea);
  }
}

void CmdBoardPolygonEdit::setPath(const Path& path, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setPath(path) && immediate) {
    mPolygon.setPath(path);
  }
}

void CmdBoardPolygonEdit::translate(const Point& deltaPos,
                                    bool immediate) noexcept {
  setPath(mNewData.getPath().translated(deltaPos), immediate);
}

void CmdBoardPolygonEdit::snapToGrid(const PositiveLength& gridInterval,
                                     bool immediate) noexcept {
  setPath(mNewData.getPath().mappedToGrid(gridInterval), immediate);
}

void CmdBoardPolygonEdit::rotate(const Angle& angle, const Point& center,
                                 bool immediate) noexcept {
  setPath(mNewData.getPath().rotated(angle, center), immediate);
}

void CmdBoardPolygonEdit::mirrorGeometry(Qt::Orientation orientation,
                                         const Point& center,
                                         bool immediate) noexcept {
  setPath(mNewData.getPath().mirrored(orientation, center), immediate);
}

void CmdBoardPolygonEdit::mirrorLayer(bool immediate) noexcept {
  setLayer(mNewData.getLayer().mirrored(), immediate);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardPolygonEdit::performExecute() {
  performRedo();  // can throw
  return (mNewData != mOldData);
}

void CmdBoardPolygonEdit::performUndo() {
  mPolygon.setLayer(mOldData.getLayer());
  mPolygon.setLineWidth(mOldData.getLineWidth());
  mPolygon.setIsFilled(mOldData.isFilled());
  mPolygon.setIsGrabArea(mOldData.isGrabArea());
  mPolygon.setPath(mOldData.getPath());
}

void CmdBoardPolygonEdit::performRedo() {
  mPolygon.setLayer(mNewData.getLayer());
  mPolygon.setLineWidth(mNewData.getLineWidth());
  mPolygon.setIsFilled(mNewData.isFilled());
  mPolygon.setIsGrabArea(mNewData.isGrabArea());
  mPolygon.setPath(mNewData.getPath());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
