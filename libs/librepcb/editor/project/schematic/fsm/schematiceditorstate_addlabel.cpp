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

#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/toolbox.h>

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
    mAutoOrientation(false),
    mCurrentNetLabelRotation(Angle::deg0()),
    mCurrentNetLabelMirrored(false),
    mCurrentBusLabelRotation(Angle::deg0()),
    mCurrentBusLabelMirrored(false),
    mCurrentNetLabel(nullptr),
    mNetLabelEditCmd(nullptr),
    mCurrentBusLabel(nullptr),
    mBusLabelEditCmd(nullptr) {
  // Restore client settings.
  QSettings cs;
  mAutoOrientation =
      cs.value("schematic_editor/add_label/auto_orientation", true).toBool();
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
    setAutoOrientation(false);  // Switch back to manual mode.
    mNetLabelEditCmd->rotate(rotation, mCurrentNetLabel->getPosition(), true);
    mCurrentNetLabelRotation = mCurrentNetLabel->getRotation();
    emit rotationChanged(mCurrentNetLabelRotation);
    return true;
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mBusLabelEditCmd->rotate(rotation, mCurrentBusLabel->getPosition(), true);
    mCurrentBusLabelRotation = mCurrentBusLabel->getRotation();
    emit rotationChanged(mCurrentBusLabelRotation);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddLabel::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mNetLabelEditCmd) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mNetLabelEditCmd->mirror(orientation, mCurrentNetLabel->getPosition(),
                             true);
    mCurrentNetLabelRotation = mCurrentNetLabel->getRotation();
    mCurrentNetLabelMirrored = mCurrentNetLabel->getMirrored();
    emit rotationChanged(mCurrentNetLabelRotation);
    emit mirroredChanged(mCurrentNetLabelMirrored);
    return true;
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mBusLabelEditCmd->mirror(orientation, mCurrentBusLabel->getPosition(),
                             true);
    mCurrentBusLabelRotation = mCurrentBusLabel->getRotation();
    mCurrentBusLabelMirrored = mCurrentBusLabel->getMirrored();
    emit rotationChanged(mCurrentBusLabelRotation);
    emit mirroredChanged(mCurrentBusLabelMirrored);
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

  return processRotate(Angle::deg90());
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

Angle SchematicEditorState_AddLabel::getRotation() const noexcept {
  return mCurrentBusLabel ? mCurrentBusLabelRotation : mCurrentNetLabelRotation;
}

void SchematicEditorState_AddLabel::setRotation(
    const Angle& rotation) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mNetLabelEditCmd &&
      (rotation != mCurrentNetLabelRotation)) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mNetLabelEditCmd->setRotation(rotation, true);
    mCurrentNetLabelRotation = rotation;
    emit rotationChanged(rotation);
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd &&
             (rotation != mCurrentBusLabelRotation)) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mBusLabelEditCmd->setRotation(rotation, true);
    mCurrentBusLabelRotation = rotation;
    emit rotationChanged(rotation);
  }
}

bool SchematicEditorState_AddLabel::getMirrored() const noexcept {
  return mCurrentBusLabel ? mCurrentBusLabelMirrored : mCurrentNetLabelMirrored;
}

void SchematicEditorState_AddLabel::setMirrored(bool mirrored) noexcept {
  if (mUndoCmdActive && mCurrentNetLabel && mNetLabelEditCmd &&
      (mirrored != mCurrentNetLabelMirrored)) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mNetLabelEditCmd->setMirrored(mirrored, true);
    mCurrentNetLabelMirrored = mirrored;
    emit mirroredChanged(mirrored);
  } else if (mUndoCmdActive && mCurrentBusLabel && mBusLabelEditCmd &&
             (mirrored != mCurrentBusLabelMirrored)) {
    setAutoOrientation(false);  // Switch back to manual mode.
    mBusLabelEditCmd->setMirrored(mirrored, true);
    mCurrentBusLabelMirrored = mirrored;
    emit mirroredChanged(mirrored);
  }
}

void SchematicEditorState_AddLabel::setAutoOrientation(bool enabled) noexcept {
  if (enabled == mAutoOrientation) {
    return;
  }

  mAutoOrientation = enabled;
  emit autoOrientationChanged(mAutoOrientation);
  updateLabelAutoOrientation();

  // Save client settings.
  QSettings cs;
  cs.setValue("schematic_editor/add_label/auto_orientation", mAutoOrientation);
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
                   mCurrentBusLabelRotation, mCurrentBusLabelMirrored));
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
                   mCurrentNetLabelRotation, mCurrentNetLabelMirrored));
      CmdSchematicNetLabelAdd* cmdAdd = new CmdSchematicNetLabelAdd(*netLabel);
      mContext.undoStack.appendToCmdGroup(cmdAdd);
      mCurrentNetLabel = netLabel;
      mNetLabelEditCmd = new CmdSchematicNetLabelEdit(*mCurrentNetLabel);

      // Highlight all elements of the current netsignal.
      mAdapter.fsmCrossProbe(
          {&line->getNetLine().getNetSegment().getNetSignal()});
    } else {
      return false;
    }

    // Apply auto-orientation, if enabled.
    updateLabelAutoOrientation();

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
    updateLabelAutoOrientation();
    return true;
  } else if (mUndoCmdActive && mBusLabelEditCmd) {
    mBusLabelEditCmd->setPosition(pos.mappedToGrid(getGridInterval()), true);
    updateLabelAutoOrientation();
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
    mAdapter.fsmCrossProbe();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

void SchematicEditorState_AddLabel::updateLabelAutoOrientation() noexcept {
  if (!mAutoOrientation) {
    return;
  }

  // Note: In auto-orientation mode, Align the label with the lines it is
  // attached to (if any). This should be consistent to what the wire and bus
  // tools do when automatically placing a label while drawing a line.
  if (mCurrentNetLabel && mNetLabelEditCmd) {
    const Point pos = mCurrentNetLabel->getNetSegment().calcNearestPoint(
        mCurrentNetLabel->getPosition());
    QVector<std::pair<Point, Point>> lines;
    const QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPos(
        pos, FindFlag::NetLines | FindFlag::AcceptNearestWithinGrid);
    for (const auto& item : items) {
      if (const auto line = std::dynamic_pointer_cast<SGI_NetLine>(item)) {
        if (line->getNetLine().getNetSegment() ==
            mCurrentNetLabel->getNetSegment()) {
          lines.append({line->getNetLine().getP1().getPosition(),
                        line->getNetLine().getP2().getPosition()});
        }
      }
    }
    if (const auto orientation = calcAutoOrientation(pos, lines)) {
      if (orientation->first != mCurrentNetLabelRotation) {
        mNetLabelEditCmd->setRotation(orientation->first, true);
        mCurrentNetLabelRotation = orientation->first;
        emit rotationChanged(orientation->first);
      }
      if (orientation->second != mCurrentNetLabelMirrored) {
        mNetLabelEditCmd->setMirrored(orientation->second, true);
        mCurrentNetLabelMirrored = orientation->second;
        emit mirroredChanged(orientation->second);
      }
    }
  } else if (mCurrentBusLabel && mBusLabelEditCmd) {
    const Point pos = mCurrentBusLabel->getBusSegment().calcNearestPoint(
        mCurrentBusLabel->getPosition());
    QVector<std::pair<Point, Point>> lines;
    const QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPos(
        pos, FindFlag::BusLines | FindFlag::AcceptNearestWithinGrid);
    for (const auto& item : items) {
      if (const auto line = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
        if (line->getBusLine().getBusSegment() ==
            mCurrentBusLabel->getBusSegment()) {
          lines.append({line->getBusLine().getP1().getPosition(),
                        line->getBusLine().getP2().getPosition()});
        }
      }
    }
    if (const auto orientation = calcAutoOrientation(pos, lines)) {
      if (orientation->first != mCurrentBusLabelRotation) {
        mBusLabelEditCmd->setRotation(orientation->first, true);
        mCurrentBusLabelRotation = orientation->first;
        emit rotationChanged(orientation->first);
      }
      if (orientation->second != mCurrentBusLabelMirrored) {
        mBusLabelEditCmd->setMirrored(orientation->second, true);
        mCurrentBusLabelMirrored = orientation->second;
        emit mirroredChanged(orientation->second);
      }
    }
  }
}

std::optional<std::pair<Angle, bool>>
    SchematicEditorState_AddLabel::calcAutoOrientation(
        const Point& pos,
        const QVector<std::pair<Point, Point>>& lines) noexcept {
  std::optional<std::pair<Angle, bool>> result;
  for (const auto& [p1, p2] : lines) {
    // Determine the direction from the label position towards the line
    // endpoint which is farther away.
    const Point dirPos =
        ((p1 - pos).getLength() >= (p2 - pos).getLength()) ? p1 : p2;
    const Angle dir =
        Toolbox::angleBetweenPoints(pos, dirPos).rounded(Angle::deg90());
    const bool mirror = dir.mappedTo0_360deg() >= Angle::deg180();
    const std::pair<Angle, bool> candidate = {
        mirror ? (dir + Angle::deg180()) : dir,
        mirror,
    };
    // If there are multiple lines, prefer those with horizontal direction.
    if (candidate.first.mappedTo180deg() == Angle::deg0()) {
      return candidate;
    } else if (!result) {
      result = candidate;
    }
  }
  return result;
}

bool SchematicEditorState_AddLabel::abortCommand(bool showErrMsgBox) noexcept {
  try {
    mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
    mAdapter.fsmCrossProbe();
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
