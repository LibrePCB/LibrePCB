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
#include "schematiceditorstate_addlabel.h"

#include "../../../undostack.h"
#include "../../cmd/cmdschematicbuslabeladd.h"
#include "../../cmd/cmdschematicbuslabeledit.h"
#include "../../cmd/cmdschematicnetlabeladd.h"
#include "../../cmd/cmdschematicnetlabeledit.h"
#include "../graphicsitems/sgi_busline.h"
#include "../graphicsitems/sgi_netline.h"

#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
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

SchematicEditorState_AddLabel::SchematicEditorState_AddLabel(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mUndoCmdActive(false),
    mCurrentNetLabel(nullptr),
    mNetLabelEditCmd(nullptr),
    mCurrentBusLabel(nullptr),
    mBusLabelEditCmd(nullptr) {
}

SchematicEditorState_AddLabel::~SchematicEditorState_AddLabel() noexcept {
  Q_ASSERT(mUndoCmdActive == false);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_AddLabel::entry() noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddLabel::exit() noexcept {
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

bool SchematicEditorState_AddLabel::processRotate(
    const Angle& rotation) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mNetLabelEditCmd) {
    mNetLabelEditCmd->rotate(rotation, mCurrentNetLabel->getPosition(), true);
    return true;
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd) {
    mBusLabelEditCmd->rotate(rotation, mCurrentBusLabel->getPosition(), true);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddLabel::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mNetLabelEditCmd) {
    mNetLabelEditCmd->mirror(orientation, mCurrentNetLabel->getPosition(),
                             true);
    return true;
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd) {
    mBusLabelEditCmd->mirror(orientation, mCurrentBusLabel->getPosition(),
                             true);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddLabel::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return updateLabel(e.scenePos);
}

bool SchematicEditorState_AddLabel::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mUndoCmdActive) {
    return fixLabel(e.scenePos);
  } else {
    return addLabel(e.scenePos);
  }
}

bool SchematicEditorState_AddLabel::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  if (mUndoCmdActive) {
    return fixLabel(e.scenePos);
  } else {
    return addLabel(e.scenePos);
  }
}

bool SchematicEditorState_AddLabel::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);

  // Note: Always accept the event if we are placing a net label! When ignoring
  // the event, the state machine will abort the tool by a right click!
  if (mUndoCmdActive && mCurrentNetLabel && mNetLabelEditCmd) {
    mNetLabelEditCmd->rotate(Angle::deg90(), mCurrentNetLabel->getPosition(),
                             true);
    return true;
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd) {
    mBusLabelEditCmd->rotate(Angle::deg90(), mCurrentBusLabel->getPosition(),
                             true);
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_AddLabel::addLabel(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mUndoCmdActive == false);

  try {
    if (std::shared_ptr<SGI_BusLine> line = findItemAtPos<SGI_BusLine>(
            pos, FindFlag::BusLines | FindFlag::AcceptNearestWithinGrid)) {
      mContext.undoStack.beginCmdGroup(tr("Add Bus Label to Schematic"));
      mUndoCmdActive = true;

      SI_BusLabel* busLabel = new SI_BusLabel(
          line->getBusLine().getBusSegment(),
          NetLabel(Uuid::createRandom(), pos.mappedToGrid(getGridInterval()),
                   Angle::deg0(), false));
      CmdSchematicBusLabelAdd* cmdAdd = new CmdSchematicBusLabelAdd(*busLabel);
      mContext.undoStack.appendToCmdGroup(cmdAdd);
      mCurrentBusLabel = busLabel;
      mBusLabelEditCmd = new CmdSchematicBusLabelEdit(*mCurrentBusLabel);
    } else if (std::shared_ptr<SGI_NetLine> line = findItemAtPos<SGI_NetLine>(
                   pos,
                   FindFlag::NetLines | FindFlag::AcceptNearestWithinGrid)) {
      mContext.undoStack.beginCmdGroup(tr("Add Net Label to Schematic"));
      mUndoCmdActive = true;

      SI_NetLabel* netLabel = new SI_NetLabel(
          line->getNetLine().getNetSegment(),
          NetLabel(Uuid::createRandom(), pos.mappedToGrid(getGridInterval()),
                   Angle::deg0(), false));
      CmdSchematicNetLabelAdd* cmdAdd = new CmdSchematicNetLabelAdd(*netLabel);
      mContext.undoStack.appendToCmdGroup(cmdAdd);
      mCurrentNetLabel = netLabel;
      mNetLabelEditCmd = new CmdSchematicNetLabelEdit(*mCurrentNetLabel);

      // Highlight all elements of the current netsignal.
      mAdapter.fsmSetHighlightedNetSignals(
          {&line->getNetLine().getNetSegment().getNetSignal()});
    } else {
      return false;
    }

    // Allow some actions.
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features(
        SchematicEditorFsmAdapter::Feature::Rotate |
        SchematicEditorFsmAdapter::Feature::Mirror));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool SchematicEditorState_AddLabel::updateLabel(const Point& pos) noexcept {
  if (mUndoCmdActive && mNetLabelEditCmd) {
    mNetLabelEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), true);
    return true;
  } else if (mUndoCmdActive && mBusLabelEditCmd) {
    mBusLabelEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), true);
    return true;
  } else {
    return false;
  }
}

bool SchematicEditorState_AddLabel::fixLabel(const Point& pos) noexcept {
  if (!mUndoCmdActive) return false;

  try {
    if (mNetLabelEditCmd) {
      mNetLabelEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), false);
      mContext.undoStack.appendToCmdGroup(mNetLabelEditCmd);
    } else if (mBusLabelEditCmd) {
      mBusLabelEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), false);
      mContext.undoStack.appendToCmdGroup(mBusLabelEditCmd);
    }
    mContext.undoStack.commitCmdGroup();
    mUndoCmdActive = false;
    mCurrentNetLabel = nullptr;
    mNetLabelEditCmd = nullptr;
    mCurrentBusLabel = nullptr;
    mBusLabelEditCmd = nullptr;
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
    mAdapter.fsmSetHighlightedNetSignals({});
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool SchematicEditorState_AddLabel::abortCommand(bool showErrMsgBox) noexcept {
  try {
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
    mAdapter.fsmSetHighlightedNetSignals({});
    if (mUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();  // can throw
      mUndoCmdActive = false;
    }
    mCurrentNetLabel = nullptr;
    mNetLabelEditCmd = nullptr;
    mCurrentBusLabel = nullptr;
    mBusLabelEditCmd = nullptr;
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
