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
#include "schematiceditorstate_drawbus.h"

#include "../../../editorcommandset.h"
#include "../../../undocommandgroup.h"
#include "../../../undostack.h"
#include "../../cmd/cmdbusadd.h"
#include "../../cmd/cmdchangebusofschematicbussegment.h"
#include "../../cmd/cmdcombineschematicbussegments.h"
#include "../../cmd/cmdschematicbuslabeladd.h"
#include "../../cmd/cmdschematicbussegmentadd.h"
#include "../../cmd/cmdschematicbussegmentaddelements.h"
#include "../../cmd/cmdschematicbussegmentremoveelements.h"
#include "../../cmd/cmdsimplifyschematicsegments.h"
#include "../graphicsitems/sgi_busjunction.h"
#include "../graphicsitems/sgi_busline.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>

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

SchematicEditorState_DrawBus::SchematicEditorState_DrawBus(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mCircuit(context.project.getCircuit()),
    mSubState(SubState::IDLE),
    mCurrentWireMode(WireMode::HV),
    mCursorPos(),
    mFixedStartAnchor(nullptr),
    mCurrentSegment(nullptr),
    mPositioningLine1(nullptr),
    mPositioningJunction1(nullptr),
    mPositioningLine2(nullptr),
    mPositioningJunction2(nullptr),
    mPositioningLabel(nullptr) {
}

SchematicEditorState_DrawBus::~SchematicEditorState_DrawBus() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_DrawBus::entry() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);

  mPreSelectedBus = std::nullopt;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_DrawBus::exit() noexcept {
  // abort the currently active command
  if (mSubState != SubState::IDLE) {
    abortPositioning(true, true);
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_DrawBus::processAbortCommand() noexcept {
  if (mSubState == SubState::POSITIONING_JUNCTION) {
    return abortPositioning(true, true);
  }

  return false;
}

bool SchematicEditorState_DrawBus::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  switch (e.key) {
    case Qt::Key_Shift: {
      if (mSubState == SubState::POSITIONING_JUNCTION) {
        updateJunctionPositions(false);
        return true;
      }
      break;
    }

    default: {
      break;
    }
  }

  return false;
}

bool SchematicEditorState_DrawBus::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  switch (e.key) {
    case Qt::Key_Shift: {
      if (mSubState == SubState::POSITIONING_JUNCTION) {
        updateJunctionPositions(true);
        return true;
      }
      break;
    }

    default: {
      break;
    }
  }

  return false;
}

bool SchematicEditorState_DrawBus::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  mCursorPos = e.scenePos;

  if (mSubState == SubState::POSITIONING_JUNCTION) {
    const bool snap = !e.modifiers.testFlag(Qt::ShiftModifier);
    updateJunctionPositions(snap);
    return true;
  }

  return false;
}

bool SchematicEditorState_DrawBus::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mCursorPos = e.scenePos;
  const bool snap = !e.modifiers.testFlag(Qt::ShiftModifier);

  if (mSubState == SubState::IDLE) {
    // start adding junctions/lines
    return startPositioning(*scene, snap);
  } else if (mSubState == SubState::POSITIONING_JUNCTION) {
    // fix the current point and add a new point + line
    return addNextJunction(*scene, snap);
  }

  return false;
}

bool SchematicEditorState_DrawBus::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mCursorPos = e.scenePos;
  const bool snap = !e.modifiers.testFlag(Qt::ShiftModifier);

  if (mSubState == SubState::POSITIONING_JUNCTION) {
    // fix the current point and add a new point + line
    return addNextJunction(*scene, snap);
  }

  return false;
}

bool SchematicEditorState_DrawBus::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  mCursorPos = e.scenePos;

  if (mSubState == SubState::POSITIONING_JUNCTION) {
    setWireMode(static_cast<WireMode>((static_cast<int>(mCurrentWireMode) + 1) %
                                      static_cast<int>(WireMode::_COUNT)));

    // Always accept the event if we are drawing a wire! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void SchematicEditorState_DrawBus::selectBus(
    const std::optional<Uuid> uuid) noexcept {
  mPreSelectedBus = uuid;
}

void SchematicEditorState_DrawBus::setWireMode(WireMode mode) noexcept {
  if (mode != mCurrentWireMode) {
    mCurrentWireMode = mode;
    emit wireModeChanged(mCurrentWireMode);
  }

  if (mSubState == SubState::POSITIONING_JUNCTION) {
    updateJunctionPositions(true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_DrawBus::startPositioning(
    SchematicGraphicsScene& scene, bool snap,
    SI_BusJunction* fixedPoint) noexcept {
  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState::IDLE);
    mContext.undoStack.beginCmdGroup(tr("Draw Wire"));
    mSubState = SubState::POSITIONING_JUNCTION;
    mPositioningJunction1 = nullptr;
    mPositioningLine1 = nullptr;
    mPositioningJunction2 = nullptr;
    mPositioningLine2 = nullptr;
    mPositioningLabel = nullptr;

    // determine the fixed anchor (create one if it doesn't exist already)
    Bus* bus = nullptr;
    mCurrentSegment = nullptr;
    Point pos = mCursorPos.mappedToGrid(getGridInterval());
    if (snap || fixedPoint) {
      std::shared_ptr<QGraphicsItem> item = findItem(mCursorPos);
      if (fixedPoint) {
        mFixedStartAnchor = fixedPoint;
        mCurrentSegment = &fixedPoint->getBusSegment();
        pos = fixedPoint->getPosition();
      } else if (auto junction =
                     std::dynamic_pointer_cast<SGI_BusJunction>(item)) {
        mFixedStartAnchor = &junction->getBusJunction();
        mCurrentSegment = &junction->getBusJunction().getBusSegment();
        pos = junction->getBusJunction().getPosition();
      } else if (auto line = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
        // split line
        mCurrentSegment = &line->getBusLine().getBusSegment();
        std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
            new CmdSchematicBusSegmentAddElements(*mCurrentSegment));
        mFixedStartAnchor = cmdAdd->addJunction(Toolbox::nearestPointOnLine(
            pos, line->getBusLine().getP1().getPosition(),
            line->getBusLine().getP2().getPosition()));
        cmdAdd->addLine(*mFixedStartAnchor, line->getBusLine().getP1());
        cmdAdd->addLine(*mFixedStartAnchor, line->getBusLine().getP2());
        mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
        std::unique_ptr<CmdSchematicBusSegmentRemoveElements> cmdRemove(
            new CmdSchematicBusSegmentRemoveElements(*mCurrentSegment));
        cmdRemove->removeLine(line->getBusLine());
        mContext.undoStack.appendToCmdGroup(cmdRemove.release());  // can throw
      }
    }

    // If no bus under cursor but manually chosen, use it.
    bool addLabel = false;
    if ((!fixedPoint) && (!bus) && mPreSelectedBus) {
      bus = mCircuit.getBuses().value(*mPreSelectedBus);
      addLabel = bus ? true : false;
    }

    // create new bus if none found
    if ((!mCurrentSegment) && (!bus)) {
      // add new bus
      CmdBusAdd* cmd = new CmdBusAdd(mCircuit);
      mContext.undoStack.appendToCmdGroup(cmd);  // can throw
      bus = cmd->getBus();
      Q_ASSERT(bus);
    }

    // create new bus segment if none found
    if (!mCurrentSegment) {
      // add bus segment
      Q_ASSERT(bus);
      CmdSchematicBusSegmentAdd* cmd =
          new CmdSchematicBusSegmentAdd(scene.getSchematic(), *bus);
      mContext.undoStack.appendToCmdGroup(cmd);  // can throw
      mCurrentSegment = cmd->getSegment();
    }

    // add junction if none found
    Q_ASSERT(mCurrentSegment);
    CmdSchematicBusSegmentAddElements* cmd =
        new CmdSchematicBusSegmentAddElements(*mCurrentSegment);
    if (!mFixedStartAnchor) {
      mFixedStartAnchor = cmd->addJunction(pos);
    }
    Q_ASSERT(mFixedStartAnchor);

    // add more junctions & lines
    mPositioningJunction1 = cmd->addJunction(pos);  // second junction
    mPositioningLine1 =
        cmd->addLine(*mFixedStartAnchor, *mPositioningJunction1);  // first line
    mPositioningJunction2 = cmd->addJunction(pos);  // third junction
    mPositioningLine2 = cmd->addLine(*mPositioningJunction1,
                                     *mPositioningJunction2);  // second line
    mContext.undoStack.appendToCmdGroup(cmd);  // can throw

    // Add label if required.
    if (addLabel) {
      mPositioningLabel = new SI_BusLabel(
          *mCurrentSegment,
          NetLabel(Uuid::createRandom(), pos, Angle::deg0(), false));
      CmdSchematicBusLabelAdd* cmdLabel =
          new CmdSchematicBusLabelAdd(*mPositioningLabel);
      mContext.undoStack.appendToCmdGroup(cmdLabel);  // can throw
    }

    // properly place the new junctions/line according the current wire mode
    updateJunctionPositions(snap);

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    if (mSubState != SubState::IDLE) {
      abortPositioning(false, false);
    }
    return false;
  }
}

bool SchematicEditorState_DrawBus::addNextJunction(
    SchematicGraphicsScene& scene, bool snap) noexcept {
  Q_ASSERT(mSubState == SubState::POSITIONING_JUNCTION);

  // Snap to the item under the cursor and make sure the lines are up to date.
  Point pos = updateJunctionPositions(snap);

  // abort if p2 == p0 (no line drawn)
  if (pos == mFixedStartAnchor->getPosition()) {
    abortPositioning(true, true);
    return false;
  } else {
    bool finishCommand = false;

    try {
      // create a new undo command group to make all changes atomic
      QScopedPointer<UndoCommandGroup> cmdGroup(
          new UndoCommandGroup("Add Schematic Bus Line"));

      // remove p1 if p1 == p0 || p1 == p2
      if ((mPositioningJunction1->getPosition() ==
           mFixedStartAnchor->getPosition()) ||
          (mPositioningJunction1->getPosition() ==
           mPositioningJunction2->getPosition())) {
        std::unique_ptr<CmdSchematicBusSegmentRemoveElements> cmdRemove(
            new CmdSchematicBusSegmentRemoveElements(
                mPositioningJunction1->getBusSegment()));
        cmdRemove->removeJunction(*mPositioningJunction1);
        cmdRemove->removeLine(*mPositioningLine1);
        cmdRemove->removeLine(*mPositioningLine2);
        std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
            new CmdSchematicBusSegmentAddElements(
                mPositioningJunction1->getBusSegment()));
        mPositioningLine2 =
            cmdAdd->addLine(*mFixedStartAnchor, *mPositioningJunction2);
        mContext.undoStack.appendToCmdGroup(cmdAdd.release());
        mContext.undoStack.appendToCmdGroup(cmdRemove.release());
      }

      // find anchor under cursor
      SI_BusJunction* otherAnchor = nullptr;
      SI_BusSegment* otherSegment = nullptr;
      if (snap) {
        std::shared_ptr<QGraphicsItem> item =
            findItem(pos,
                     {
                         scene.getBusJunctions().value(mPositioningJunction2),
                         scene.getBusLines().value(mPositioningLine2),
                     });
        if (auto junction = std::dynamic_pointer_cast<SGI_BusJunction>(item)) {
          otherAnchor = &junction->getBusJunction();
          otherSegment = &junction->getBusJunction().getBusSegment();
        } else if (auto line = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
          // split line
          otherSegment = &line->getBusLine().getBusSegment();
          std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
              new CmdSchematicBusSegmentAddElements(*otherSegment));
          otherAnchor = cmdAdd->addJunction(pos);
          cmdAdd->addLine(*otherAnchor, line->getBusLine().getP1());
          cmdAdd->addLine(*otherAnchor, line->getBusLine().getP2());
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
          std::unique_ptr<CmdSchematicBusSegmentRemoveElements> cmdRemove(
              new CmdSchematicBusSegmentRemoveElements(*otherSegment));
          cmdRemove->removeLine(line->getBusLine());
          mContext.undoStack.appendToCmdGroup(
              cmdRemove.release());  // can throw
        }
      }

      // if anchor found under the cursor, replace "mPositioningJunction2" with
      // it
      if (otherAnchor) {
        if ((!otherSegment) ||
            (otherSegment == &mPositioningJunction2->getBusSegment())) {
          std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
              new CmdSchematicBusSegmentAddElements(
                  mPositioningJunction2->getBusSegment()));
          SI_BusJunction* np2 =
              mPositioningLine2->getOtherPoint(*mPositioningJunction2);
          if (!np2) throw LogicError(__FILE__, __LINE__);
          cmdAdd->addLine(*otherAnchor, *np2);
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
          std::unique_ptr<CmdSchematicBusSegmentRemoveElements> cmdRemove(
              new CmdSchematicBusSegmentRemoveElements(
                  mPositioningJunction2->getBusSegment()));
          cmdRemove->removeJunction(*mPositioningJunction2);
          cmdRemove->removeLine(*mPositioningLine2);
          mContext.undoStack.appendToCmdGroup(
              cmdRemove.release());  // can throw
        } else {
          // Change bus if needed.
          Bus* thisBus = &mPositioningJunction2->getBusSegment().getBus();
          Bus* otherBus = &otherSegment->getBus();
          if (thisBus != otherBus) {
            Bus* resultingBus = nullptr;
            SI_BusSegment* busSegmentToChangeSignal = nullptr;
            if (otherBus->hasAutoName() && (!thisBus->hasAutoName())) {
              resultingBus = &mPositioningJunction2->getBusSegment().getBus();
              busSegmentToChangeSignal = otherSegment;
            } else {
              resultingBus = &otherSegment->getBus();
              busSegmentToChangeSignal =
                  &mPositioningJunction2->getBusSegment();
            }
            mContext.undoStack.appendToCmdGroup(
                new CmdChangeBusOfSchematicBusSegment(*busSegmentToChangeSignal,
                                                      *resultingBus));
          }
          // Combine both bus segments.
          mContext.undoStack.appendToCmdGroup(
              new CmdCombineSchematicBusSegments(
                  mPositioningJunction2->getBusSegment(),
                  *mPositioningJunction2, *otherSegment, *otherAnchor));
          mCurrentSegment = otherSegment;
        }
        finishCommand = true;
      } else {
        finishCommand = false;
      }
    } catch (const UserCanceled& e) {
      return false;
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      return false;
    }

    try {
      // finish the current command
      mContext.undoStack.commitCmdGroup();
      mSubState = SubState::IDLE;

      // abort or start a new command
      if (finishCommand) {
        mContext.undoStack.beginCmdGroup(QString());  // this is ugly!
        abortPositioning(true, true);
        return false;
      } else {
        return startPositioning(scene, snap, mPositioningJunction2);
      }
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      if (mSubState != SubState::IDLE) {
        abortPositioning(false, false);
      }
      return false;
    }
  }
}

bool SchematicEditorState_DrawBus::abortPositioning(
    bool showErrMsgBox, bool simplifySegment) noexcept {
  bool success = false;

  SI_BusSegment* segment = simplifySegment ? mCurrentSegment : nullptr;

  try {
    mAdapter.fsmSetHighlightedNetSignals({});
    mSubState = SubState::IDLE;
    mFixedStartAnchor = nullptr;
    mCurrentSegment = nullptr;
    mPositioningLine1 = nullptr;
    mPositioningLine2 = nullptr;
    mPositioningJunction1 = nullptr;
    mPositioningJunction2 = nullptr;
    mPositioningLabel = nullptr;
    mContext.undoStack.abortCmdGroup();  // can throw
    success = true;
  } catch (const Exception& e) {
    if (showErrMsgBox)
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }

  if (segment) {
    try {
      // Probably we don't have to simplify any net segments?
      mContext.undoStack.execCmd(
          new CmdSimplifySchematicSegments({}, {segment}));
    } catch (const Exception& e) {
      qCritical() << "Failed to simplify schematic segments:" << e.getMsg();
    }
  }

  return success;
}

std::shared_ptr<QGraphicsItem> SchematicEditorState_DrawBus::findItem(
    const Point& pos,
    const QVector<std::shared_ptr<QGraphicsItem>>& except) noexcept {
  // Only find pins which are connected to a component signal!
  return findItemAtPos<QGraphicsItem>(pos,
                                      FindFlag::BusJunctions |
                                          FindFlag::BusLines |
                                          FindFlag::AcceptNearestWithinGrid,
                                      except);
}

Point SchematicEditorState_DrawBus::updateJunctionPositions(
    bool snap) noexcept {
  // Find anchor under cursor.
  Point pos = mCursorPos.mappedToGrid(getGridInterval());
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (snap && scene) {
    std::shared_ptr<QGraphicsItem> item =
        findItem(mCursorPos,
                 {
                     scene->getBusJunctions().value(mPositioningJunction1),
                     scene->getBusJunctions().value(mPositioningJunction2),
                     scene->getBusLines().value(mPositioningLine1),
                     scene->getBusLines().value(mPositioningLine2),
                 });
    if (auto junction = std::dynamic_pointer_cast<SGI_BusJunction>(item)) {
      pos = junction->getBusJunction().getPosition();
    } else if (auto line = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
      pos = Toolbox::nearestPointOnLine(
          pos, line->getBusLine().getP1().getPosition(),
          line->getBusLine().getP2().getPosition());
    } else if (item) {
      qCritical() << "Found item below cursor, but it has an unexpected type!";
    }
  }

  // All pointers should be valid, but let's be on the safe side.
  Point middlePos = pos;
  if (mFixedStartAnchor && mPositioningJunction1) {
    middlePos = calcMiddlePointPos(mFixedStartAnchor->getPosition(), pos,
                                   mCurrentWireMode);
    mPositioningJunction1->setPosition(middlePos);
  }
  if (mPositioningJunction2) {
    mPositioningJunction2->setPosition(pos);
  }
  if (mFixedStartAnchor) {
    const Point startPos = mFixedStartAnchor->getPosition();
    updateLabelPosition(startPos, (middlePos != startPos) ? middlePos : pos);
  }
  return pos;
}

void SchematicEditorState_DrawBus::updateLabelPosition(
    const Point& pos, const Point& dirPos) noexcept {
  if (mPositioningLabel) {
    const Angle dir =
        Toolbox::angleBetweenPoints(pos, dirPos).rounded(Angle::deg90());
    ;
    const bool mirror = dir.mappedTo0_360deg() >= Angle::deg180();
    mPositioningLabel->setPosition(pos);
    mPositioningLabel->setRotation(mirror ? (dir + Angle::deg180()) : dir);
    mPositioningLabel->setMirrored(mirror);
  }
}

Point SchematicEditorState_DrawBus::calcMiddlePointPos(
    const Point& p1, const Point p2, WireMode mode) const noexcept {
  Point delta = p2 - p1;
  switch (mode) {
    case WireMode::HV:
      return Point(p2.getX(), p1.getY());
    case WireMode::VH:
      return Point(p1.getX(), p2.getY());
    case WireMode::Deg9045:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(
            p2.getX() - delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1),
            p1.getY());
      else
        return Point(
            p1.getX(),
            p2.getY() - delta.getX().abs() * (delta.getY() >= 0 ? 1 : -1));
    case WireMode::Deg4590:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(
            p1.getX() + delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1),
            p2.getY());
      else
        return Point(
            p2.getX(),
            p1.getY() + delta.getX().abs() * (delta.getY() >= 0 ? 1 : -1));
    case WireMode::Straight:
      return p1;
    default:
      Q_ASSERT(false);
      return Point();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
