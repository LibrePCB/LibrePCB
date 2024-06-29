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
#include "cmddragselectedsymbolitems.h"

#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdtextedit.h"
#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/textgraphicsitem.h"
#include "../../widgets/graphicsview.h"
#include "../cmd/cmdsymbolpinedit.h"
#include "../sym/symbolgraphicsitem.h"
#include "../sym/symbolpingraphicsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDragSelectedSymbolItems::CmdDragSelectedSymbolItems(
    const SymbolEditorState::Context& context) noexcept
  : UndoCommandGroup(tr("Drag Symbol Elements")),
    mContext(context),
    mCenterPos(0, 0),
    mDeltaPos(0, 0),
    mDeltaRot(0),
    mMirrored(false),
    mSnappedToGrid(false),
    mHasOffTheGridElements(false) {
  int count = 0;
  PositiveLength grid = mContext.graphicsView.getGridInterval();

  QList<std::shared_ptr<SymbolPinGraphicsItem>> pins =
      context.symbolGraphicsItem.getSelectedPins();
  foreach (const std::shared_ptr<SymbolPinGraphicsItem>& pin, pins) {
    Q_ASSERT(pin);
    mPinEditCmds.append(new CmdSymbolPinEdit(pin->getPtr()));
    mCenterPos += pin->getObj().getPosition();
    if (!pin->getObj().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<std::shared_ptr<CircleGraphicsItem>> circles =
      context.symbolGraphicsItem.getSelectedCircles();
  foreach (const std::shared_ptr<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getObj()));
    mCenterPos += circle->getObj().getCenter();
    if (!circle->getObj().getCenter().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<std::shared_ptr<PolygonGraphicsItem>> polygons =
      context.symbolGraphicsItem.getSelectedPolygons();
  foreach (const std::shared_ptr<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    mPolygonEditCmds.append(new CmdPolygonEdit(polygon->getObj()));
    foreach (const Vertex& vertex, polygon->getObj().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      if (!vertex.getPos().isOnGrid(grid)) {
        mHasOffTheGridElements = true;
      }
      ++count;
    }
  }

  QList<std::shared_ptr<TextGraphicsItem>> texts =
      context.symbolGraphicsItem.getSelectedTexts();
  foreach (const std::shared_ptr<TextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdTextEdit(text->getObj()));
    mCenterPos += text->getObj().getPosition();
    if (!text->getObj().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  // Note: If only 1 item is selected, use its exact position as center.
  if (count > 1) {
    mCenterPos /= count;
    mCenterPos.mapToGrid(grid);
  }
}

CmdDragSelectedSymbolItems::~CmdDragSelectedSymbolItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int CmdDragSelectedSymbolItems::getSelectedItemsCount() const noexcept {
  return mPinEditCmds.count() + mCircleEditCmds.count() +
      mPolygonEditCmds.count() + mTextEditCmds.count();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedSymbolItems::snapToGrid() noexcept {
  PositiveLength grid = mContext.graphicsView.getGridInterval();
  foreach (CmdSymbolPinEdit* cmd, mPinEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  mSnappedToGrid = true;
}

void CmdDragSelectedSymbolItems::setDeltaToStartPos(
    const Point& delta) noexcept {
  translate(delta - mDeltaPos);
}

void CmdDragSelectedSymbolItems::translate(const Point& deltaPos) noexcept {
  if (!deltaPos.isOrigin()) {
    foreach (CmdSymbolPinEdit* cmd, mPinEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdTextEdit* cmd, mTextEditCmds) {
      cmd->translate(deltaPos, true);
    }
    mDeltaPos += deltaPos;
    mCenterPos += deltaPos;
  }
}

void CmdDragSelectedSymbolItems::rotate(const Angle& angle) noexcept {
  foreach (CmdSymbolPinEdit* cmd, mPinEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  mDeltaRot += angle;
}

void CmdDragSelectedSymbolItems::mirror(Qt::Orientation orientation) noexcept {
  foreach (CmdSymbolPinEdit* cmd, mPinEditCmds) {
    cmd->mirror(orientation, mCenterPos, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->mirror(orientation, mCenterPos, true);
  }
  mMirrored = !mMirrored;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedSymbolItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaRot == 0) && (!mMirrored) &&
      (!mSnappedToGrid)) {
    // no movement required --> discard all move commands
    deleteAllCommands();
    return false;
  }

  // move all child commands to parent class
  while (mPinEditCmds.count() > 0) {
    appendChild(mPinEditCmds.takeLast());
  }
  while (mCircleEditCmds.count() > 0) {
    appendChild(mCircleEditCmds.takeLast());
  }
  while (mPolygonEditCmds.count() > 0) {
    appendChild(mPolygonEditCmds.takeLast());
  }
  while (mTextEditCmds.count() > 0) {
    appendChild(mTextEditCmds.takeLast());
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdDragSelectedSymbolItems::deleteAllCommands() noexcept {
  qDeleteAll(mPinEditCmds);
  mPinEditCmds.clear();
  qDeleteAll(mCircleEditCmds);
  mCircleEditCmds.clear();
  qDeleteAll(mPolygonEditCmds);
  mPolygonEditCmds.clear();
  qDeleteAll(mTextEditCmds);
  mTextEditCmds.clear();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
