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
#include "cmdmoveselectedsymbolitems.h"

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

CmdMoveSelectedSymbolItems::CmdMoveSelectedSymbolItems(
    const SymbolEditorState::Context& context, const Point& startPos) noexcept
  : UndoCommandGroup(tr("Move Symbol Elements")),
    mContext(context),
    mStartPos(startPos),
    mDeltaPos(0, 0) {
  QList<QSharedPointer<SymbolPinGraphicsItem>> pins =
      context.symbolGraphicsItem.getSelectedPins();
  foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin, pins) {
    Q_ASSERT(pin);
    mPinEditCmds.append(new CmdSymbolPinEdit(pin->getPin()));
  }

  QList<QSharedPointer<CircleGraphicsItem>> circles =
      context.symbolGraphicsItem.getSelectedCircles();
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getCircle()));
  }

  QList<QSharedPointer<PolygonGraphicsItem>> polygons =
      context.symbolGraphicsItem.getSelectedPolygons();
  foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    mPolygonEditCmds.append(new CmdPolygonEdit(polygon->getPolygon()));
  }

  QList<QSharedPointer<TextGraphicsItem>> texts =
      context.symbolGraphicsItem.getSelectedTexts();
  foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdTextEdit(text->getText()));
  }
}

CmdMoveSelectedSymbolItems::~CmdMoveSelectedSymbolItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdMoveSelectedSymbolItems::setCurrentPosition(const Point& pos) noexcept {
  Point delta = pos - mStartPos;
  delta.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());

  if (delta != mDeltaPos) {
    // move selected elements
    foreach (CmdSymbolPinEdit* cmd, mPinEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdTextEdit* cmd, mTextEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    mDeltaPos = delta;
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdMoveSelectedSymbolItems::performExecute() {
  if (mDeltaPos.isOrigin()) {
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

void CmdMoveSelectedSymbolItems::deleteAllCommands() noexcept {
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
