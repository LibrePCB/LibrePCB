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

#include <librepcb/common/geometry/cmd/cmdcircleedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdtextedit.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/library/sym/cmd/cmdsymbolpinedit.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/library/sym/symbolpingraphicsitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
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
  PositiveLength grid = mContext.graphicsView.getGridProperties().getInterval();

  QList<QSharedPointer<SymbolPinGraphicsItem>> pins =
      context.symbolGraphicsItem.getSelectedPins();
  foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin, pins) {
    Q_ASSERT(pin);
    mPinEditCmds.append(new CmdSymbolPinEdit(pin->getPin()));
    mCenterPos += pin->getPin().getPosition();
    if (!pin->getPin().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<QSharedPointer<CircleGraphicsItem>> circles =
      context.symbolGraphicsItem.getSelectedCircles();
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getCircle()));
    mCenterPos += circle->getCircle().getCenter();
    if (!circle->getCircle().getCenter().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<QSharedPointer<PolygonGraphicsItem>> polygons =
      context.symbolGraphicsItem.getSelectedPolygons();
  foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    mPolygonEditCmds.append(new CmdPolygonEdit(polygon->getPolygon()));
    foreach (const Vertex& vertex,
             polygon->getPolygon().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      if (!vertex.getPos().isOnGrid(grid)) {
        mHasOffTheGridElements = true;
      }
      ++count;
    }
  }

  QList<QSharedPointer<TextGraphicsItem>> texts =
      context.symbolGraphicsItem.getSelectedTexts();
  foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdTextEdit(text->getText()));
    mCenterPos += text->getText().getPosition();
    if (!text->getText().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  mCenterPos /= qMax(count, 1);
  mCenterPos.mapToGrid(grid);
}

CmdDragSelectedSymbolItems::~CmdDragSelectedSymbolItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedSymbolItems::snapToGrid() noexcept {
  PositiveLength grid = mContext.graphicsView.getGridProperties().getInterval();
  foreach (CmdSymbolPinEdit* cmd, mPinEditCmds) { cmd->snapToGrid(grid, true); }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) { cmd->snapToGrid(grid, true); }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) { cmd->snapToGrid(grid, true); }
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
}  // namespace library
}  // namespace librepcb
