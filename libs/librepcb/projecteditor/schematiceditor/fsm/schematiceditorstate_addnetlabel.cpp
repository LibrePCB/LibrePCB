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
#include "schematiceditorstate_addnetlabel.h"

#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeladd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeledit.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/schematic.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditorState_AddNetLabel::SchematicEditorState_AddNetLabel(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mUndoCmdActive(false),
    mCurrentNetLabel(nullptr),
    mEditCmd(nullptr) {
}

SchematicEditorState_AddNetLabel::~SchematicEditorState_AddNetLabel() noexcept {
  Q_ASSERT(mUndoCmdActive == false);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_AddNetLabel::entry() noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  // change the cursor
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool SchematicEditorState_AddNetLabel::exit() noexcept {
  if (mUndoCmdActive) {
    try {
      mContext.undoStack.abortCmdGroup();
      mUndoCmdActive = false;
    } catch (Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      return false;
    }
  }

  // change the cursor
  mContext.editorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_AddNetLabel::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  return updateLabel(Point::fromPx(e.scenePos()));
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  Point pos = Point::fromPx(e.scenePos());

  if (mUndoCmdActive) {
    return fixLabel(pos);
  } else {
    return addLabel(*schematic, pos);
  }
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  Point pos = Point::fromPx(e.scenePos());

  if (mUndoCmdActive) {
    return fixLabel(pos);
  } else {
    return addLabel(*schematic, pos);
  }
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mEditCmd) {
    // Only rotate net label if cursor was not moved during click
    if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
      mEditCmd->rotate(Angle::deg90(), mCurrentNetLabel->getPosition(), true);
    }

    // Always accept the event if we are placing a net label! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

bool SchematicEditorState_AddNetLabel::processSwitchToSchematicPage(
    int index) noexcept {
  Q_UNUSED(index);
  return !mUndoCmdActive;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_AddNetLabel::addLabel(Schematic& schematic,
                                                const Point& pos) noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  try {
    QList<SI_NetLine*> netlinesUnderCursor =
        schematic.getNetLinesAtScenePos(pos);
    if (netlinesUnderCursor.isEmpty()) return false;
    SI_NetSegment& netsegment = netlinesUnderCursor.first()->getNetSegment();

    mContext.undoStack.beginCmdGroup(tr("Add net label to schematic"));
    mUndoCmdActive = true;
    CmdSchematicNetLabelAdd* cmdAdd = new CmdSchematicNetLabelAdd(
        netsegment, pos.mappedToGrid(getGridInterval()), Angle::deg0(),
        Alignment());
    mContext.undoStack.appendToCmdGroup(cmdAdd);
    mCurrentNetLabel = cmdAdd->getNetLabel();
    mEditCmd = new CmdSchematicNetLabelEdit(*mCurrentNetLabel);
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mContext.undoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
    }
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_AddNetLabel::updateLabel(const Point& pos) noexcept {
  if (mUndoCmdActive) {
    mEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), true);
    return true;
  } else {
    return false;
  }
}

bool SchematicEditorState_AddNetLabel::fixLabel(const Point& pos) noexcept {
  if (!mUndoCmdActive) return false;

  try {
    mEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), false);
    mContext.undoStack.appendToCmdGroup(mEditCmd);
    mContext.undoStack.commitCmdGroup();
    mUndoCmdActive = false;
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mContext.undoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
    }
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
