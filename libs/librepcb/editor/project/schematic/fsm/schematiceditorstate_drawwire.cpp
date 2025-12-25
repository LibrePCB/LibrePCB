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
#include "schematiceditorstate_drawwire.h"

#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../cmd/cmdchangenetsignalofschematicnetsegment.h"
#include "../../cmd/cmdcombineschematicnetsegments.h"
#include "../../cmd/cmdcompsiginstsetnetsignal.h"
#include "../../cmd/cmdnetclassadd.h"
#include "../../cmd/cmdnetsignaladd.h"
#include "../../cmd/cmdnetsignaledit.h"
#include "../../cmd/cmdschematicbussegmentaddelements.h"
#include "../../cmd/cmdschematicbussegmentremoveelements.h"
#include "../../cmd/cmdschematicnetlabeladd.h"
#include "../../cmd/cmdschematicnetsegmentadd.h"
#include "../../cmd/cmdschematicnetsegmentaddelements.h"
#include "../../cmd/cmdschematicnetsegmentremoveelements.h"
#include "../../cmd/cmdsimplifyschematicsegments.h"
#include "../graphicsitems/sgi_busjunction.h"
#include "../graphicsitems/sgi_busline.h"
#include "../graphicsitems/sgi_netline.h"
#include "../graphicsitems/sgi_netpoint.h"
#include "../graphicsitems/sgi_symbolpin.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>

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

SchematicEditorState_DrawWire::SchematicEditorState_DrawWire(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mCircuit(context.project.getCircuit()),
    mSubState(SubState::IDLE),
    mCurrentWireMode(WireMode::HV),
    mCursorPos(),
    mFixedStartAnchor(nullptr),
    mCurrentNetSegment(nullptr),
    mPositioningNetLine1(nullptr),
    mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr),
    mPositioningNetPoint2(nullptr),
    mPositioningNetLabel(nullptr) {
}

SchematicEditorState_DrawWire::~SchematicEditorState_DrawWire() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_DrawWire::entry() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_DrawWire::exit() noexcept {
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

bool SchematicEditorState_DrawWire::processAbortCommand() noexcept {
  if (mSubState == SubState::POSITIONING_NETPOINT) {
    return abortPositioning(true, true);
  }

  return false;
}

bool SchematicEditorState_DrawWire::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  switch (e.key) {
    case Qt::Key_Shift: {
      if (mSubState == SubState::POSITIONING_NETPOINT) {
        updateNetpointPositions(false);
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

bool SchematicEditorState_DrawWire::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  switch (e.key) {
    case Qt::Key_Shift: {
      if (mSubState == SubState::POSITIONING_NETPOINT) {
        updateNetpointPositions(true);
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

bool SchematicEditorState_DrawWire::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  mCursorPos = e.scenePos;

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    const bool snap = !e.modifiers.testFlag(Qt::ShiftModifier);
    updateNetpointPositions(snap);
    return true;
  }

  return false;
}

bool SchematicEditorState_DrawWire::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mCursorPos = e.scenePos;
  const bool snap = !e.modifiers.testFlag(Qt::ShiftModifier);
  const bool interactive = !e.modifiers.testFlag(Qt::ControlModifier);

  if (mSubState == SubState::IDLE) {
    // start adding netpoints/netlines
    return startPositioning(*scene, snap, interactive);
  } else if (mSubState == SubState::POSITIONING_NETPOINT) {
    // fix the current point and add a new point + line
    return addNextNetPoint(*scene, snap, interactive);
  }

  return false;
}

bool SchematicEditorState_DrawWire::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mCursorPos = e.scenePos;
  const bool snap = !e.modifiers.testFlag(Qt::ShiftModifier);
  const bool interactive = !e.modifiers.testFlag(Qt::ControlModifier);

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    // fix the current point and add a new point + line
    return addNextNetPoint(*scene, snap, interactive);
  }

  return false;
}

bool SchematicEditorState_DrawWire::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  mCursorPos = e.scenePos;

  if (mSubState == SubState::POSITIONING_NETPOINT) {
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

void SchematicEditorState_DrawWire::setWireMode(WireMode mode) noexcept {
  if (mode != mCurrentWireMode) {
    mCurrentWireMode = mode;
    emit wireModeChanged(mCurrentWireMode);
  }

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    updateNetpointPositions(true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_DrawWire::startPositioning(
    SchematicGraphicsScene& scene, bool snap, bool interactive,
    SI_NetPoint* fixedPoint) noexcept {
  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState::IDLE);
    mContext.undoStack.beginCmdGroup(tr("Draw Wire"));
    mSubState = SubState::POSITIONING_NETPOINT;
    mPositioningNetPoint1 = nullptr;
    mPositioningNetLine1 = nullptr;
    mPositioningNetPoint2 = nullptr;
    mPositioningNetLine2 = nullptr;
    mPositioningNetLabel = nullptr;

    // determine the fixed anchor (create one if it doesn't exist already)
    NetSignal* netsignal = nullptr;
    mCurrentNetSegment = nullptr;
    std::optional<CircuitIdentifier> forcedNetName;
    Point pos = mCursorPos.mappedToGrid(getGridInterval());
    if (snap || fixedPoint) {
      std::shared_ptr<QGraphicsItem> item = findItem(mCursorPos);
      if (fixedPoint) {
        mFixedStartAnchor = fixedPoint;
        mCurrentNetSegment = &fixedPoint->getNetSegment();
        pos = fixedPoint->getPosition();
      } else if (auto bj = std::dynamic_pointer_cast<SGI_BusJunction>(item)) {
        mFixedStartAnchor = &bj->getBusJunction();
        pos = bj->getBusJunction().getPosition();
      } else if (auto netpoint =
                     std::dynamic_pointer_cast<SGI_NetPoint>(item)) {
        mFixedStartAnchor = &netpoint->getNetPoint();
        mCurrentNetSegment = &netpoint->getNetPoint().getNetSegment();
        pos = netpoint->getNetPoint().getPosition();
      } else if (auto pin = std::dynamic_pointer_cast<SGI_SymbolPin>(item)) {
        mFixedStartAnchor = &pin->getPin();
        mCurrentNetSegment = pin->getPin().getNetSegmentOfLines();
        netsignal = pin->getPin().getCompSigInstNetSignal();
        pos = pin->getPin().getPosition();
        QString name =
            pin->getPin().getComponentSignalInstance().getForcedNetSignalName();
        try {
          if (!name.isEmpty()) {
            forcedNetName = CircuitIdentifier(name);  // can throw
          }
        } catch (const Exception& e) {
          QMessageBox::warning(
              parentWidget(), tr("Invalid net name"),
              tr("Could not apply the forced net name because '%1' is "
                 "not a valid net name.")
                  .arg(name));
        }
      } else if (auto bl = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
        // split bus line
        SI_BusSegment& segment = bl->getBusLine().getBusSegment();
        std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
            new CmdSchematicBusSegmentAddElements(segment));
        SI_BusJunction* bj = cmdAdd->addJunction(Toolbox::nearestPointOnLine(
            pos, bl->getBusLine().getP1().getPosition(),
            bl->getBusLine().getP2().getPosition()));
        cmdAdd->addLine(*bj, bl->getBusLine().getP1());
        cmdAdd->addLine(*bj, bl->getBusLine().getP2());
        mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
        std::unique_ptr<CmdSchematicBusSegmentRemoveElements> cmdRemove(
            new CmdSchematicBusSegmentRemoveElements(segment));
        cmdRemove->removeLine(bl->getBusLine());
        mContext.undoStack.appendToCmdGroup(cmdRemove.release());  // can throw
        mFixedStartAnchor = bj;
      } else if (auto netline = std::dynamic_pointer_cast<SGI_NetLine>(item)) {
        // split netline
        mCurrentNetSegment = &netline->getNetLine().getNetSegment();
        std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
            new CmdSchematicNetSegmentAddElements(*mCurrentNetSegment));
        mFixedStartAnchor = cmdAdd->addNetPoint(Toolbox::nearestPointOnLine(
            pos, netline->getNetLine().getP1().getPosition(),
            netline->getNetLine().getP2().getPosition()));
        cmdAdd->addNetLine(*mFixedStartAnchor, netline->getNetLine().getP1());
        cmdAdd->addNetLine(*mFixedStartAnchor, netline->getNetLine().getP2());
        mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
        std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
            new CmdSchematicNetSegmentRemoveElements(*mCurrentNetSegment));
        cmdRemove->removeNetLine(netline->getNetLine());
        mContext.undoStack.appendToCmdGroup(cmdRemove.release());  // can throw
      }
    }

    // find netsignal if name is given
    if (forcedNetName) {
      netsignal = mCircuit.getNetSignalByName(**forcedNetName);
    }

    // If clicking on a bus (without pressing CTRL), show a menu to choose
    // the net signal to break out from the bus.
    bool addNetLabel = false;
    if (SI_BusJunction* bj = dynamic_cast<SI_BusJunction*>(mFixedStartAnchor)) {
      if (interactive) {
        if (auto ns = determineNetForBusMember(*bj)) {
          netsignal = *ns;
          addNetLabel = true;
        } else {
          throw UserCanceled(__FILE__, __LINE__);
        }
      }
    }

    // create new netsignal if none found
    if ((!mCurrentNetSegment) && (!netsignal)) {
      // get or add netclass with the name "default"
      NetClass* netclass = mCircuit.getNetClassByName(ElementName("default"));
      if (!netclass) {
        CmdNetClassAdd* cmd =
            new CmdNetClassAdd(mCircuit, ElementName("default"));
        mContext.undoStack.appendToCmdGroup(cmd);  // can throw
        netclass = cmd->getNetClass();
        Q_ASSERT(netclass);
      }
      // add new netsignal
      CmdNetSignalAdd* cmd =
          new CmdNetSignalAdd(mCircuit, *netclass, forcedNetName);
      mContext.undoStack.appendToCmdGroup(cmd);  // can throw
      netsignal = cmd->getNetSignal();
      Q_ASSERT(netsignal);
    }

    // create new netsegment if none found
    if (!mCurrentNetSegment) {
      // connect pin if needed
      if (SI_SymbolPin* pin = dynamic_cast<SI_SymbolPin*>(mFixedStartAnchor)) {
        mContext.undoStack.appendToCmdGroup(new CmdCompSigInstSetNetSignal(
            pin->getComponentSignalInstance(), netsignal));
      }
      // add net segment
      Q_ASSERT(netsignal);
      CmdSchematicNetSegmentAdd* cmd =
          new CmdSchematicNetSegmentAdd(scene.getSchematic(), *netsignal);
      mContext.undoStack.appendToCmdGroup(cmd);  // can throw
      mCurrentNetSegment = cmd->getNetSegment();
      // Add net label, if required.
      if (addNetLabel) {
        mPositioningNetLabel = new SI_NetLabel(
            *mCurrentNetSegment,
            NetLabel(Uuid::createRandom(), pos, Angle::deg0(), false));
        CmdSchematicNetLabelAdd* cmdLabel =
            new CmdSchematicNetLabelAdd(*mPositioningNetLabel);
        mContext.undoStack.appendToCmdGroup(cmdLabel);
      }
    }

    // add netpoint if none found
    Q_ASSERT(mCurrentNetSegment);
    CmdSchematicNetSegmentAddElements* cmd =
        new CmdSchematicNetSegmentAddElements(*mCurrentNetSegment);
    if (!mFixedStartAnchor) {
      mFixedStartAnchor = cmd->addNetPoint(pos);
    }
    Q_ASSERT(mFixedStartAnchor);

    // add more netpoints & netlines
    mPositioningNetPoint1 = cmd->addNetPoint(pos);
    mPositioningNetLine1 =
        cmd->addNetLine(*mFixedStartAnchor, *mPositioningNetPoint1);
    mPositioningNetPoint2 = cmd->addNetPoint(pos);
    mPositioningNetLine2 =
        cmd->addNetLine(*mPositioningNetPoint1, *mPositioningNetPoint2);
    mContext.undoStack.appendToCmdGroup(cmd);  // can throw

    // properly place the new netpoints/netlines according the current wire mode
    updateNetpointPositions(snap);

    // Highlight all elements of the current netsignal.
    mAdapter.fsmSetHighlightedNetSignals({&mCurrentNetSegment->getNetSignal()});

    return true;
  } catch (const UserCanceled& e) {
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  if (mSubState != SubState::IDLE) {
    abortPositioning(false, false);
  }
  return false;
}

bool SchematicEditorState_DrawWire::addNextNetPoint(
    SchematicGraphicsScene& scene, bool snap, bool interactive) noexcept {
  Q_ASSERT(mSubState == SubState::POSITIONING_NETPOINT);

  // Snap to the item under the cursor and make sure the lines are up to date.
  Point pos = updateNetpointPositions(snap);

  // abort if p2 == p0 (no line drawn)
  if (pos == mFixedStartAnchor->getPosition()) {
    abortPositioning(true, true);
    return false;
  } else {
    bool finishCommand = false;

    try {
      // create a new undo command group to make all changes atomic
      QScopedPointer<UndoCommandGroup> cmdGroup(
          new UndoCommandGroup("Draw Wire"));

      // remove p1 if p1 == p0 || p1 == p2
      if ((mPositioningNetPoint1->getPosition() ==
           mFixedStartAnchor->getPosition()) ||
          (mPositioningNetPoint1->getPosition() ==
           mPositioningNetPoint2->getPosition())) {
        std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
            new CmdSchematicNetSegmentRemoveElements(
                mPositioningNetPoint1->getNetSegment()));
        cmdRemove->removeNetPoint(*mPositioningNetPoint1);
        cmdRemove->removeNetLine(*mPositioningNetLine1);
        cmdRemove->removeNetLine(*mPositioningNetLine2);
        std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
            new CmdSchematicNetSegmentAddElements(
                mPositioningNetPoint1->getNetSegment()));
        mPositioningNetLine2 =
            cmdAdd->addNetLine(*mFixedStartAnchor, *mPositioningNetPoint2);
        mContext.undoStack.appendToCmdGroup(cmdAdd.release());
        mContext.undoStack.appendToCmdGroup(cmdRemove.release());
      }

      // find anchor under cursor
      SI_NetLineAnchor* otherAnchor = nullptr;
      SI_NetSegment* otherNetSegment = nullptr;
      QString otherForcedNetName;
      if (snap) {
        std::shared_ptr<QGraphicsItem> item =
            findItem(pos,
                     {
                         scene.getNetPoints().value(mPositioningNetPoint2),
                         scene.getNetLines().value(mPositioningNetLine2),
                     });
        if (auto bj = std::dynamic_pointer_cast<SGI_BusJunction>(item)) {
          otherAnchor = &bj->getBusJunction();
        } else if (auto netpoint =
                       std::dynamic_pointer_cast<SGI_NetPoint>(item)) {
          otherAnchor = &netpoint->getNetPoint();
          otherNetSegment = &netpoint->getNetPoint().getNetSegment();
        } else if (auto pin = std::dynamic_pointer_cast<SGI_SymbolPin>(item)) {
          otherAnchor = &pin->getPin();
          otherNetSegment = pin->getPin().getNetSegmentOfLines();
          // connect pin if needed
          if (!otherNetSegment) {
            mContext.undoStack.appendToCmdGroup(new CmdCompSigInstSetNetSignal(
                pin->getPin().getComponentSignalInstance(),
                &mPositioningNetPoint2->getNetSegment().getNetSignal()));
            otherForcedNetName = pin->getPin()
                                     .getComponentSignalInstance()
                                     .getForcedNetSignalName();
          }
        } else if (auto bl = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
          // split bus line
          SI_BusSegment& segment = bl->getBusLine().getBusSegment();
          std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
              new CmdSchematicBusSegmentAddElements(segment));
          SI_BusJunction* bj = cmdAdd->addJunction(pos);
          cmdAdd->addLine(*bj, bl->getBusLine().getP1());
          cmdAdd->addLine(*bj, bl->getBusLine().getP2());
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
          std::unique_ptr<CmdSchematicBusSegmentRemoveElements> cmdRemove(
              new CmdSchematicBusSegmentRemoveElements(segment));
          cmdRemove->removeLine(bl->getBusLine());
          mContext.undoStack.appendToCmdGroup(
              cmdRemove.release());  // can throw
          otherAnchor = bj;
        } else if (auto netline =
                       std::dynamic_pointer_cast<SGI_NetLine>(item)) {
          // split netline
          otherNetSegment = &netline->getNetLine().getNetSegment();
          std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
              new CmdSchematicNetSegmentAddElements(*otherNetSegment));
          otherAnchor = cmdAdd->addNetPoint(pos);
          cmdAdd->addNetLine(*otherAnchor, netline->getNetLine().getP1());
          cmdAdd->addNetLine(*otherAnchor, netline->getNetLine().getP2());
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
          std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
              new CmdSchematicNetSegmentRemoveElements(*otherNetSegment));
          cmdRemove->removeNetLine(netline->getNetLine());
          mContext.undoStack.appendToCmdGroup(
              cmdRemove.release());  // can throw
        }
      }

      // If clicking on a bus (without pressing CTRL), show a menu to choose
      // the net signal and add a net label to the current segment.
      if (SI_BusJunction* bj = dynamic_cast<SI_BusJunction*>(otherAnchor)) {
        SI_NetSegment& seg = mPositioningNetLine2->getNetSegment();
        if (mFixedStartAnchor && mPositioningNetPoint1 &&
            seg.getNetLabels().isEmpty()) {
          if ((!seg.getNetSignal().isNameForced()) && interactive) {
            if (auto opt = determineNetForBusMember(*bj)) {
              if (NetSignal* ns = *opt) {
                otherForcedNetName = *ns->getName();
              }
            } else {
              throw UserCanceled(__FILE__, __LINE__);
            }
          }
          mPositioningNetLabel = new SI_NetLabel(
              seg, NetLabel(Uuid::createRandom(), pos, Angle::deg0(), false));
          CmdSchematicNetLabelAdd* cmdLabel =
              new CmdSchematicNetLabelAdd(*mPositioningNetLabel);
          mContext.undoStack.appendToCmdGroup(cmdLabel);
          updateNetLabelPosition(mFixedStartAnchor->getPosition(),
                                 mPositioningNetPoint1->getPosition());
        }
      }

      // if anchor found under the cursor, replace "mPositioningNetPoint2" with
      // it
      if (otherAnchor) {
        if ((!otherNetSegment) ||
            (otherNetSegment == &mPositioningNetPoint2->getNetSegment())) {
          std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
              new CmdSchematicNetSegmentAddElements(
                  mPositioningNetPoint2->getNetSegment()));
          SI_NetLineAnchor* np2 =
              mPositioningNetLine2->getOtherPoint(*mPositioningNetPoint2);
          if (!np2) throw LogicError(__FILE__, __LINE__);
          cmdAdd->addNetLine(*otherAnchor, *np2);
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
          std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
              new CmdSchematicNetSegmentRemoveElements(
                  mPositioningNetPoint2->getNetSegment()));
          cmdRemove->removeNetPoint(*mPositioningNetPoint2);
          cmdRemove->removeNetLine(*mPositioningNetLine2);
          mContext.undoStack.appendToCmdGroup(
              cmdRemove.release());  // can throw
        } else {
          // change net signal if needed
          NetSignal* thisSignal =
              &mPositioningNetPoint2->getNetSegment().getNetSignal();
          NetSignal* otherSignal = &otherNetSegment->getNetSignal();
          if (thisSignal != otherSignal) {
            NetSignal* resultingNetSignal = nullptr;
            SI_NetSegment* netSegmentToChangeSignal = nullptr;
            if (otherNetSegment->getForcedNetNames().count() > 0) {
              resultingNetSignal = &otherNetSegment->getNetSignal();
              netSegmentToChangeSignal =
                  &mPositioningNetPoint2->getNetSegment();
            } else if (mPositioningNetPoint2->getNetSegment()
                           .getForcedNetNames()
                           .count() > 0) {
              resultingNetSignal =
                  &mPositioningNetPoint2->getNetSegment().getNetSignal();
              netSegmentToChangeSignal = otherNetSegment;
            } else if (otherSignal->hasAutoName() &&
                       (!thisSignal->hasAutoName())) {
              resultingNetSignal =
                  &mPositioningNetPoint2->getNetSegment().getNetSignal();
              netSegmentToChangeSignal = otherNetSegment;
            } else {
              resultingNetSignal = &otherNetSegment->getNetSignal();
              netSegmentToChangeSignal =
                  &mPositioningNetPoint2->getNetSegment();
            }
            mContext.undoStack.appendToCmdGroup(
                new CmdChangeNetSignalOfSchematicNetSegment(
                    *netSegmentToChangeSignal, *resultingNetSignal));
          }
          // combine both net segments
          mContext.undoStack.appendToCmdGroup(
              new CmdCombineSchematicNetSegments(
                  mPositioningNetPoint2->getNetSegment(),
                  *mPositioningNetPoint2, *otherNetSegment, *otherAnchor));
          mCurrentNetSegment = otherNetSegment;
        }
        if (!otherForcedNetName.isEmpty()) {
          // change net name if connected to a pin with forced net name
          try {
            CircuitIdentifier name =
                CircuitIdentifier(otherForcedNetName);  // can throw
            NetSignal* signal = mCircuit.getNetSignalByName(*name);
            if (signal) {
              mContext.undoStack.appendToCmdGroup(
                  new CmdChangeNetSignalOfSchematicNetSegment(
                      mPositioningNetPoint2->getNetSegment(), *signal));
            } else {
              std::unique_ptr<CmdNetSignalEdit> cmd(new CmdNetSignalEdit(
                  mCircuit,
                  mPositioningNetPoint2->getNetSegment().getNetSignal()));
              cmd->setName(name, false);
              mContext.undoStack.appendToCmdGroup(cmd.release());
            }
          } catch (const Exception& e) {
            QMessageBox::warning(
                parentWidget(), tr("Invalid net name"),
                QString(
                    tr("Could not apply the forced net name because '%1' is "
                       "not a valid net name."))
                    .arg(otherForcedNetName));
          }
        }
        finishCommand = true;
      } else {
        finishCommand = false;
      }
    } catch (const UserCanceled& e) {
      // Discard any temporary changes, e.g. the splitting of bus lines
      // or the merging of net segment lines.
      abortPositioning(false, true);
      return false;
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      abortPositioning(false, true);  // Discard any temporary changes.
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
        return startPositioning(scene, snap, true, mPositioningNetPoint2);
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

bool SchematicEditorState_DrawWire::abortPositioning(
    bool showErrMsgBox, bool simplifySegment) noexcept {
  bool success = false;

  SI_NetSegment* segment = simplifySegment ? mCurrentNetSegment : nullptr;

  try {
    mAdapter.fsmSetHighlightedNetSignals({});
    mSubState = SubState::IDLE;
    mFixedStartAnchor = nullptr;
    mCurrentNetSegment = nullptr;
    mPositioningNetLine1 = nullptr;
    mPositioningNetLine2 = nullptr;
    mPositioningNetPoint1 = nullptr;
    mPositioningNetPoint2 = nullptr;
    mPositioningNetLabel = nullptr;
    mContext.undoStack.abortCmdGroup();  // can throw
    success = true;
  } catch (const Exception& e) {
    if (showErrMsgBox)
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }

  if (segment) {
    try {
      // Not sure if we also need to simplify affected bus segments, but I
      // guess just attaching a wire to them should not require that.
      mContext.undoStack.execCmd(
          new CmdSimplifySchematicSegments({segment}, {}));
    } catch (const Exception& e) {
      qCritical() << "Failed to simplify net segments:" << e.getMsg();
    }
  }

  return success;
}

std::shared_ptr<QGraphicsItem> SchematicEditorState_DrawWire::findItem(
    const Point& pos,
    const QVector<std::shared_ptr<QGraphicsItem>>& except) noexcept {
  // Only find pins which are connected to a component signal!
  return findItemAtPos<QGraphicsItem>(
      pos,
      FindFlag::BusJunctions | FindFlag::BusLines | FindFlag::NetPoints |
          FindFlag::NetLines | FindFlag::SymbolPins |
          FindFlag::AcceptNearestWithinGrid,
      except);
}

Point SchematicEditorState_DrawWire::updateNetpointPositions(
    bool snap) noexcept {
  // Find anchor under cursor.
  Point pos = mCursorPos.mappedToGrid(getGridInterval());
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  WireMode wireMode = mCurrentWireMode;
  if (dynamic_cast<SI_BusJunction*>(mFixedStartAnchor)) {
    wireMode = WireMode::Deg4590;
  }
  if (snap && scene) {
    std::shared_ptr<QGraphicsItem> item =
        findItem(mCursorPos,
                 {
                     scene->getNetPoints().value(mPositioningNetPoint1),
                     scene->getNetPoints().value(mPositioningNetPoint2),
                     scene->getNetLines().value(mPositioningNetLine1),
                     scene->getNetLines().value(mPositioningNetLine2),
                 });
    if (auto bj = std::dynamic_pointer_cast<SGI_BusJunction>(item)) {
      pos = bj->getBusJunction().getPosition();
      wireMode = WireMode::Deg9045;
    } else if (auto bl = std::dynamic_pointer_cast<SGI_BusLine>(item)) {
      pos = Toolbox::nearestPointOnLine(pos,
                                        bl->getBusLine().getP1().getPosition(),
                                        bl->getBusLine().getP2().getPosition());
      wireMode = WireMode::Deg9045;
    } else if (auto netPoint = std::dynamic_pointer_cast<SGI_NetPoint>(item)) {
      pos = netPoint->getNetPoint().getPosition();
    } else if (auto pin = std::dynamic_pointer_cast<SGI_SymbolPin>(item)) {
      pos = pin->getPin().getPosition();
    } else if (auto netline = std::dynamic_pointer_cast<SGI_NetLine>(item)) {
      pos = Toolbox::nearestPointOnLine(
          pos, netline->getNetLine().getP1().getPosition(),
          netline->getNetLine().getP2().getPosition());
    } else if (item) {
      qCritical() << "Found item below cursor, but it has an unexpected type!";
    }
  }

  // All pointers should be valid, but let's be on the safe side.
  Point middlePos = pos;
  if (mFixedStartAnchor && mPositioningNetPoint1) {
    middlePos =
        calcMiddlePointPos(mFixedStartAnchor->getPosition(), pos, wireMode);
    mPositioningNetPoint1->setPosition(middlePos);
  }
  if (mPositioningNetPoint2) {
    mPositioningNetPoint2->setPosition(pos);
  }
  if (mFixedStartAnchor) {
    const Point startPos = mFixedStartAnchor->getPosition();
    updateNetLabelPosition(pos, (middlePos != pos) ? middlePos : startPos);
  }
  return pos;
}

void SchematicEditorState_DrawWire::updateNetLabelPosition(
    const Point& pos, const Point& dirPos) noexcept {
  if (mPositioningNetLabel) {
    const Angle dir =
        Toolbox::angleBetweenPoints(pos, dirPos).rounded(Angle::deg90());
    const bool mirror = dir.mappedTo0_360deg() >= Angle::deg180();
    mPositioningNetLabel->setPosition(pos);
    mPositioningNetLabel->setRotation(mirror ? (dir + Angle::deg180()) : dir);
    mPositioningNetLabel->setMirrored(mirror);
  }
}

Point SchematicEditorState_DrawWire::calcMiddlePointPos(
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

std::optional<NetSignal*>
    SchematicEditorState_DrawWire::determineNetForBusMember(
        SI_BusJunction& junction) const noexcept {
  const Bus& bus = junction.getBusSegment().getBus();
  QVector<NetSignal*> nets = Toolbox::toVector(bus.getConnectedNetSignals());
  Toolbox::sortNumeric(nets,
                       [](const QCollator& comp, NetSignal* a, NetSignal* b) {
                         if (a->isAnonymous() != b->isAnonymous()) {
                           return b->isAnonymous();
                         } else {
                           return comp(*a->getName(), *b->getName());
                         }
                       });
  QMenu menu;
  menu.setDefaultAction(
      menu.addAction(QIcon(":/img/actions/draw_wire.png"),
                     tr("Add New Bus Member") +
                         QString(" (%1)").arg(QCoreApplication::translate(
                             "QShortcut", "Ctrl"))));
  NetSignal* selectedNet = nullptr;
  for (NetSignal* net : nets) {
    QAction* a =
        menu.addAction(QIcon(":/img/actions/draw_wire.png"), *net->getName(),
                       [&selectedNet, net]() { selectedNet = net; });
    a->setEnabled(!net->isAnonymous());
  }
  menu.addSeparator();
  QAction* aCancel =
      menu.addAction(QIcon(":/img/actions/cancel.png"), tr("Cancel"));
  QAction* a = menu.exec(QCursor::pos());
  if ((!a) || (a == aCancel)) {
    return std::nullopt;
  }
  return selectedNet;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
