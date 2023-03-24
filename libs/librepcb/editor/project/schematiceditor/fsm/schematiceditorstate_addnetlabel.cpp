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

#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmdschematicnetlabeladd.h"
#include "../../cmd/cmdschematicnetlabeledit.h"
#include "../../projecteditor.h"
#include "../graphicsitems/sgi_netline.h"

#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/schematic.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
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

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddNetLabel::exit() noexcept {
  if (!abortCommand(true)) {
    return false;
  }

  mContext.editorGraphicsView.unsetCursor();
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
  Point pos = Point::fromPx(e.scenePos());

  if (mUndoCmdActive) {
    return fixLabel(pos);
  } else {
    return addLabel(pos);
  }
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos());

  if (mUndoCmdActive) {
    return fixLabel(pos);
  } else {
    return addLabel(pos);
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

bool SchematicEditorState_AddNetLabel::processRotate(
    const Angle& rotation) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mEditCmd) {
    mEditCmd->rotate(rotation, mCurrentNetLabel->getPosition(), true);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddNetLabel::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mEditCmd) {
    mEditCmd->mirror(orientation, mCurrentNetLabel->getPosition(), true);
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_AddNetLabel::addLabel(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mUndoCmdActive == false);
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  try {
    std::shared_ptr<SGI_NetLine> netlineUnderCursor =
        findItemAtPos<SGI_NetLine>(
            pos, FindFlag::NetLines | FindFlag::AcceptNearestWithinGrid);
    if (!netlineUnderCursor) return false;
    SI_NetSegment& netsegment =
        netlineUnderCursor->getNetLine().getNetSegment();

    mContext.undoStack.beginCmdGroup(tr("Add Net Label to Schematic"));
    mUndoCmdActive = true;
    SI_NetLabel* netLabel = new SI_NetLabel(
        netsegment,
        NetLabel(Uuid::createRandom(), pos.mappedToGrid(getGridInterval()),
                 Angle::deg0(), false));
    CmdSchematicNetLabelAdd* cmdAdd = new CmdSchematicNetLabelAdd(*netLabel);
    mContext.undoStack.appendToCmdGroup(cmdAdd);
    mCurrentNetLabel = netLabel;
    mEditCmd = new CmdSchematicNetLabelEdit(*mCurrentNetLabel);

    // Highlight all elements of the current netsignal.
    mContext.projectEditor.setHighlightedNetSignals(
        {&netsegment.getNetSignal()});

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
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
    mContext.projectEditor.clearHighlightedNetSignals();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool SchematicEditorState_AddNetLabel::abortCommand(
    bool showErrMsgBox) noexcept {
  try {
    mContext.projectEditor.clearHighlightedNetSignals();
    if (mUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();  // can throw
      mUndoCmdActive = false;
    }
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
