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
#include "../../cmd/cmdschematicnetlabeladd.h"
#include "../../cmd/cmdschematicnetlabeledit.h"
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

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddNetLabel::exit() noexcept {
  if (!abortCommand(true)) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

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

bool SchematicEditorState_AddNetLabel::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return updateLabel(e.scenePos);
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneLeftMouseButtonPressed(
        const GraphicsSceneMouseEvent& e) noexcept {
  if (mUndoCmdActive) {
    return fixLabel(e.scenePos);
  } else {
    return addLabel(e.scenePos);
  }
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  if (mUndoCmdActive) {
    return fixLabel(e.scenePos);
  } else {
    return addLabel(e.scenePos);
  }
}

bool SchematicEditorState_AddNetLabel::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);

  if (mUndoCmdActive && mCurrentNetLabel && mEditCmd) {
    mEditCmd->rotate(Angle::deg90(), mCurrentNetLabel->getPosition(), true);

    // Always accept the event if we are placing a net label! When ignoring the
    // event, the state machine will abort the tool by a right click!
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

    // Allow some actions.
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features(
        SchematicEditorFsmAdapter::Feature::Rotate |
        SchematicEditorFsmAdapter::Feature::Mirror));

    // Highlight all elements of the current netsignal.
    mAdapter.fsmSetHighlightedNetSignals({&netsegment.getNetSignal()});

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
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
    mAdapter.fsmSetHighlightedNetSignals({});
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
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
    mAdapter.fsmSetHighlightedNetSignals({});
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
