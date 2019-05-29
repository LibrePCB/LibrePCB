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
#include "cmdpolygonedit.h"

#include <librepcb/common/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPolygonEdit::CmdPolygonEdit(Polygon& polygon) noexcept
  : UndoCommand(tr("Edit polygon")),
    mPolygon(polygon),
    mOldLayerName(polygon.getLayerName()),
    mNewLayerName(mOldLayerName),
    mOldLineWidth(polygon.getLineWidth()),
    mNewLineWidth(mOldLineWidth),
    mOldIsFilled(polygon.isFilled()),
    mNewIsFilled(mOldIsFilled),
    mOldIsGrabArea(polygon.isGrabArea()),
    mNewIsGrabArea(mOldIsGrabArea),
    mOldPath(polygon.getPath()),
    mNewPath(mOldPath) {
}

CmdPolygonEdit::~CmdPolygonEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdPolygonEdit::setLayerName(const GraphicsLayerName& name,
                                  bool                     immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayerName = name;
  if (immediate) mPolygon.setLayerName(mNewLayerName);
}

void CmdPolygonEdit::setLineWidth(const UnsignedLength& width,
                                  bool                  immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLineWidth = width;
  if (immediate) mPolygon.setLineWidth(mNewLineWidth);
}

void CmdPolygonEdit::setIsFilled(bool filled, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewIsFilled = filled;
  if (immediate) mPolygon.setIsFilled(mNewIsFilled);
}

void CmdPolygonEdit::setIsGrabArea(bool grabArea, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewIsGrabArea = grabArea;
  if (immediate) mPolygon.setIsGrabArea(mNewIsGrabArea);
}

void CmdPolygonEdit::setPath(const Path& path, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPath = path;
  if (immediate) mPolygon.setPath(mNewPath);
}

void CmdPolygonEdit::translate(const Point& deltaPos, bool immediate) noexcept {
  setPath(mNewPath.translated(deltaPos), immediate);
}

void CmdPolygonEdit::rotate(const Angle& angle, const Point& center,
                            bool immediate) noexcept {
  setPath(mNewPath.rotated(angle, center), immediate);
}

void CmdPolygonEdit::mirror(const Point& center, Qt::Orientation orientation,
                            bool immediate) noexcept {
  setLayerName(
      GraphicsLayerName(GraphicsLayer::getMirroredLayerName(*mNewLayerName)),
      immediate);
  setPath(mNewPath.mirrored(orientation, center), immediate);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPolygonEdit::performExecute() {
  performRedo();  // can throw

  if (mNewLayerName != mOldLayerName) return true;
  if (mNewLineWidth != mOldLineWidth) return true;
  if (mNewIsFilled != mOldIsFilled) return true;
  if (mNewIsGrabArea != mOldIsGrabArea) return true;
  if (mNewPath != mOldPath) return true;
  return false;
}

void CmdPolygonEdit::performUndo() {
  mPolygon.setLayerName(mOldLayerName);
  mPolygon.setLineWidth(mOldLineWidth);
  mPolygon.setIsFilled(mOldIsFilled);
  mPolygon.setIsGrabArea(mOldIsGrabArea);
  mPolygon.setPath(mOldPath);
}

void CmdPolygonEdit::performRedo() {
  mPolygon.setLayerName(mNewLayerName);
  mPolygon.setLineWidth(mNewLineWidth);
  mPolygon.setIsFilled(mNewIsFilled);
  mPolygon.setIsGrabArea(mNewIsGrabArea);
  mPolygon.setPath(mNewPath);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
