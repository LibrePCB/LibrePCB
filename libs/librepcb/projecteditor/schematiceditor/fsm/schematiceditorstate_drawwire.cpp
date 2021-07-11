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

#include "../../cmd/cmdchangenetsignalofschematicnetsegment.h"
#include "../../cmd/cmdcombineschematicnetsegments.h"
#include "ui_schematiceditor.h"

#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetclassadd.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaladd.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaledit.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremoveelements.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>

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

SchematicEditorState_DrawWire::SchematicEditorState_DrawWire(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mCircuit(context.project.getCircuit()),
    mSubState(SubState::IDLE),
    mWireMode(WireMode_HV),
    mCursorPos(),
    mFixedStartAnchor(nullptr),
    mPositioningNetLine1(nullptr),
    mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr),
    mPositioningNetPoint2(nullptr) {
}

SchematicEditorState_DrawWire::~SchematicEditorState_DrawWire() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_DrawWire::entry() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);

  // clear schematic selection because selection does not make sense in this
  // state
  if (Schematic* schematic = getActiveSchematic()) {
    schematic->clearSelection();
  }

  // Add wire mode actions to the "command" toolbar
  mWireModeActions.insert(
      WireMode_HV,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_h_v.png"), ""));
  mWireModeActions.insert(
      WireMode_VH,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_v_h.png"), ""));
  mWireModeActions.insert(
      WireMode_9045,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_90_45.png"), ""));
  mWireModeActions.insert(
      WireMode_4590,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_45_90.png"), ""));
  mWireModeActions.insert(
      WireMode_Straight,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_straight.png"), ""));
  mActionSeparators.append(mContext.editorUi.commandToolbar->addSeparator());
  updateWireModeActionsCheckedState();

  // connect the wire mode actions with the slot
  // updateWireModeActionsCheckedState()
  foreach (WireMode mode, mWireModeActions.keys()) {
    connect(mWireModeActions.value(mode), &QAction::triggered, [this, mode]() {
      mWireMode = mode;
      updateWireModeActionsCheckedState();
    });
  }

  // change the cursor
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool SchematicEditorState_DrawWire::exit() noexcept {
  // abort the currently active command
  if (mSubState != SubState::IDLE) {
    abortPositioning(true);
  }

  // Remove actions / widgets from the "command" toolbar
  qDeleteAll(mWireModeActions);
  mWireModeActions.clear();
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();

  // change the cursor
  mContext.editorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_DrawWire::processAbortCommand() noexcept {
  if (mSubState == SubState::POSITIONING_NETPOINT) {
    return abortPositioning(true);
  }

  return false;
}

bool SchematicEditorState_DrawWire::processKeyPressed(
    const QKeyEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  switch (e.key()) {
    case Qt::Key_Shift: {
      if (mSubState == SubState::POSITIONING_NETPOINT) {
        updateNetpointPositions(*schematic, false);
        return true;
      }
      break;
    }

    default: { break; }
  }

  return false;
}

bool SchematicEditorState_DrawWire::processKeyReleased(
    const QKeyEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  switch (e.key()) {
    case Qt::Key_Shift: {
      if (mSubState == SubState::POSITIONING_NETPOINT) {
        updateNetpointPositions(*schematic, true);
        return true;
      }
      break;
    }

    default: { break; }
  }

  return false;
}

bool SchematicEditorState_DrawWire::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  mCursorPos = Point::fromPx(e.scenePos());

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);
    updateNetpointPositions(*schematic, snap);
    return true;
  }

  return false;
}

bool SchematicEditorState_DrawWire::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  mCursorPos = Point::fromPx(e.scenePos());
  bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);

  if (mSubState == SubState::IDLE) {
    // start adding netpoints/netlines
    return startPositioning(*schematic, snap);
  } else if (mSubState == SubState::POSITIONING_NETPOINT) {
    // fix the current point and add a new point + line
    return addNextNetPoint(*schematic, snap);
  }

  return false;
}

bool SchematicEditorState_DrawWire::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  mCursorPos = Point::fromPx(e.scenePos());
  bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    // fix the current point and add a new point + line
    return addNextNetPoint(*schematic, snap);
  }

  return false;
}

bool SchematicEditorState_DrawWire::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  mCursorPos = Point::fromPx(e.scenePos());

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    // Only switch to next wire mode if cursor was not moved during click
    if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
      mWireMode = static_cast<WireMode>(mWireMode + 1);
      if (mWireMode == WireMode_COUNT) mWireMode = static_cast<WireMode>(0);
      updateWireModeActionsCheckedState();
      bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);
      updateNetpointPositions(*schematic, snap);
    }

    // Always accept the event if we are drawing a wire! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

bool SchematicEditorState_DrawWire::processSwitchToSchematicPage(
    int index) noexcept {
  Q_UNUSED(index);
  return mSubState == SubState::IDLE;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_DrawWire::startPositioning(
    Schematic& schematic, bool snap, SI_NetPoint* fixedPoint) noexcept {
  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState::IDLE);
    mContext.undoStack.beginCmdGroup(tr("Draw Wire"));
    mSubState = SubState::POSITIONING_NETPOINT;

    // determine the fixed anchor (create one if it doesn't exist already)
    NetSignal* netsignal = nullptr;
    SI_NetSegment* netsegment = nullptr;
    tl::optional<CircuitIdentifier> forcedNetName;
    Point pos = mCursorPos.mappedToGrid(getGridInterval());
    if (snap || fixedPoint) {
      if (fixedPoint) {
        mFixedStartAnchor = fixedPoint;
        netsegment = &fixedPoint->getNetSegment();
        pos = fixedPoint->getPosition();
      } else if (SI_NetPoint* netpoint = findNetPoint(schematic, mCursorPos)) {
        mFixedStartAnchor = netpoint;
        netsegment = &netpoint->getNetSegment();
        pos = netpoint->getPosition();
      } else if (SI_SymbolPin* pin = findSymbolPin(schematic, mCursorPos)) {
        mFixedStartAnchor = pin;
        netsegment = pin->getNetSegmentOfLines();
        netsignal = pin->getCompSigInstNetSignal();
        pos = pin->getPosition();
        if (pin->getComponentSignalInstance()) {
          QString name =
              pin->getComponentSignalInstance()->getForcedNetSignalName();
          try {
            if (!name.isEmpty())
              forcedNetName = CircuitIdentifier(name);  // can throw
          } catch (const Exception& e) {
            QMessageBox::warning(
                parentWidget(), tr("Invalid net name"),
                tr("Could not apply the forced net name because '%1' is "
                   "not a valid net name.")
                    .arg(name));
          }
        }
      } else if (SI_NetLine* netline = findNetLine(schematic, mCursorPos)) {
        // split netline
        netsegment = &netline->getNetSegment();
        QScopedPointer<CmdSchematicNetSegmentAddElements> cmdAdd(
            new CmdSchematicNetSegmentAddElements(*netsegment));
        mFixedStartAnchor = cmdAdd->addNetPoint(pos);
        cmdAdd->addNetLine(*mFixedStartAnchor, netline->getStartPoint());
        cmdAdd->addNetLine(*mFixedStartAnchor, netline->getEndPoint());
        mContext.undoStack.appendToCmdGroup(cmdAdd.take());  // can throw
        QScopedPointer<CmdSchematicNetSegmentRemoveElements> cmdRemove(
            new CmdSchematicNetSegmentRemoveElements(*netsegment));
        cmdRemove->removeNetLine(*netline);
        mContext.undoStack.appendToCmdGroup(cmdRemove.take());  // can throw
      }
    }

    // find netsignal if name is given
    if (forcedNetName) {
      netsignal = mCircuit.getNetSignalByName(**forcedNetName);
    }

    // create new netsignal if none found
    if ((!netsegment) && (!netsignal)) {
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
    if (!netsegment) {
      // connect pin if needed
      if (SI_SymbolPin* pin = dynamic_cast<SI_SymbolPin*>(mFixedStartAnchor)) {
        Q_ASSERT(pin->getComponentSignalInstance());
        mContext.undoStack.appendToCmdGroup(new CmdCompSigInstSetNetSignal(
            *pin->getComponentSignalInstance(), netsignal));
      }
      // add net segment
      Q_ASSERT(netsignal);
      CmdSchematicNetSegmentAdd* cmd =
          new CmdSchematicNetSegmentAdd(schematic, *netsignal);
      mContext.undoStack.appendToCmdGroup(cmd);  // can throw
      netsegment = cmd->getNetSegment();
    }

    // add netpoint if none found
    Q_ASSERT(netsegment);
    CmdSchematicNetSegmentAddElements* cmd =
        new CmdSchematicNetSegmentAddElements(*netsegment);
    if (!mFixedStartAnchor) {
      mFixedStartAnchor = cmd->addNetPoint(pos);
    }
    Q_ASSERT(mFixedStartAnchor);

    // add more netpoints & netlines
    SI_NetPoint* p2 = cmd->addNetPoint(pos);
    Q_ASSERT(p2);  // second netpoint
    SI_NetLine* l1 = cmd->addNetLine(*mFixedStartAnchor, *p2);
    Q_ASSERT(l1);  // first netline
    SI_NetPoint* p3 = cmd->addNetPoint(pos);
    Q_ASSERT(p3);  // third netpoint
    SI_NetLine* l2 = cmd->addNetLine(*p2, *p3);
    Q_ASSERT(l2);  // second netline
    mContext.undoStack.appendToCmdGroup(cmd);  // can throw

    // update members
    mPositioningNetPoint1 = p2;
    mPositioningNetLine1 = l1;
    mPositioningNetPoint2 = p3;
    mPositioningNetLine2 = l2;

    // properly place the new netpoints/netlines according the current wire mode
    updateNetpointPositions(schematic, snap);

    // highlight all elements of the current netsignal
    mCircuit.setHighlightedNetSignal(&netsegment->getNetSignal());

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    if (mSubState != SubState::IDLE) {
      abortPositioning(false);
    }
    return false;
  }
}

bool SchematicEditorState_DrawWire::addNextNetPoint(Schematic& schematic,
                                                    bool snap) noexcept {
  Q_ASSERT(mSubState == SubState::POSITIONING_NETPOINT);

  // Snap to the item under the cursor and make sure the lines are up to date.
  Point pos = updateNetpointPositions(schematic, snap);

  // abort if p2 == p0 (no line drawn)
  if (pos == mFixedStartAnchor->getPosition()) {
    abortPositioning(true);
    return false;
  } else {
    bool finishCommand = false;

    try {
      // create a new undo command group to make all changes atomic
      QScopedPointer<UndoCommandGroup> cmdGroup(
          new UndoCommandGroup("Add schematic netline"));

      // remove p1 if p1 == p0 || p1 == p2
      if ((mPositioningNetPoint1->getPosition() ==
           mFixedStartAnchor->getPosition()) ||
          (mPositioningNetPoint1->getPosition() ==
           mPositioningNetPoint2->getPosition())) {
        QScopedPointer<CmdSchematicNetSegmentRemoveElements> cmdRemove(
            new CmdSchematicNetSegmentRemoveElements(
                mPositioningNetPoint1->getNetSegment()));
        cmdRemove->removeNetPoint(*mPositioningNetPoint1);
        cmdRemove->removeNetLine(*mPositioningNetLine1);
        cmdRemove->removeNetLine(*mPositioningNetLine2);
        QScopedPointer<CmdSchematicNetSegmentAddElements> cmdAdd(
            new CmdSchematicNetSegmentAddElements(
                mPositioningNetPoint1->getNetSegment()));
        mPositioningNetLine2 =
            cmdAdd->addNetLine(*mFixedStartAnchor, *mPositioningNetPoint2);
        mContext.undoStack.appendToCmdGroup(cmdAdd.take());
        mContext.undoStack.appendToCmdGroup(cmdRemove.take());
      }

      // find anchor under cursor
      SI_NetLineAnchor* otherAnchor = nullptr;
      SI_NetSegment* otherNetSegment = nullptr;
      QString otherForcedNetName;
      if (snap) {
        if (SI_NetPoint* netpoint =
                findNetPoint(schematic, pos, mPositioningNetPoint2)) {
          otherAnchor = netpoint;
          otherNetSegment = &netpoint->getNetSegment();
        } else if (SI_SymbolPin* pin = findSymbolPin(schematic, pos)) {
          otherAnchor = pin;
          otherNetSegment = pin->getNetSegmentOfLines();
          // connect pin if needed
          if (!otherNetSegment) {
            Q_ASSERT(pin->getComponentSignalInstance());
            mContext.undoStack.appendToCmdGroup(new CmdCompSigInstSetNetSignal(
                *pin->getComponentSignalInstance(),
                &mPositioningNetPoint2->getNetSignalOfNetSegment()));
            otherForcedNetName =
                pin->getComponentSignalInstance()->getForcedNetSignalName();
          }
        } else if (SI_NetLine* netline =
                       findNetLine(schematic, pos, mPositioningNetLine2)) {
          // split netline
          otherNetSegment = &netline->getNetSegment();
          QScopedPointer<CmdSchematicNetSegmentAddElements> cmdAdd(
              new CmdSchematicNetSegmentAddElements(*otherNetSegment));
          otherAnchor = cmdAdd->addNetPoint(pos);
          cmdAdd->addNetLine(*otherAnchor, netline->getStartPoint());
          cmdAdd->addNetLine(*otherAnchor, netline->getEndPoint());
          mContext.undoStack.appendToCmdGroup(cmdAdd.take());  // can throw
          QScopedPointer<CmdSchematicNetSegmentRemoveElements> cmdRemove(
              new CmdSchematicNetSegmentRemoveElements(*otherNetSegment));
          cmdRemove->removeNetLine(*netline);
          mContext.undoStack.appendToCmdGroup(cmdRemove.take());  // can throw
        }
      }

      // if anchor found under the cursor, replace "mPositioningNetPoint2" with
      // it
      if (otherAnchor) {
        if ((!otherNetSegment) ||
            (otherNetSegment == &mPositioningNetPoint2->getNetSegment())) {
          QScopedPointer<CmdSchematicNetSegmentAddElements> cmdAdd(
              new CmdSchematicNetSegmentAddElements(
                  mPositioningNetPoint2->getNetSegment()));
          cmdAdd->addNetLine(*otherAnchor,
                             mPositioningNetLine2->getStartPoint());
          mContext.undoStack.appendToCmdGroup(cmdAdd.take());  // can throw
          QScopedPointer<CmdSchematicNetSegmentRemoveElements> cmdRemove(
              new CmdSchematicNetSegmentRemoveElements(
                  mPositioningNetPoint2->getNetSegment()));
          cmdRemove->removeNetPoint(*mPositioningNetPoint2);
          cmdRemove->removeNetLine(*mPositioningNetLine2);
          mContext.undoStack.appendToCmdGroup(cmdRemove.take());  // can throw
        } else {
          // change net signal if needed
          NetSignal* thisSignal =
              &mPositioningNetPoint2->getNetSignalOfNetSegment();
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
                  &mPositioningNetPoint2->getNetSignalOfNetSegment();
              netSegmentToChangeSignal = otherNetSegment;
            } else if (otherSignal->hasAutoName() &&
                       (!thisSignal->hasAutoName())) {
              resultingNetSignal =
                  &mPositioningNetPoint2->getNetSignalOfNetSegment();
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
        }
        if (!otherForcedNetName.isEmpty()) {
          // change net name if connected to a pin with forced net name
          try {
            CircuitIdentifier name =
                CircuitIdentifier(otherForcedNetName);  // can throw
            NetSignal* signal =
                schematic.getProject().getCircuit().getNetSignalByName(*name);
            if (signal) {
              mContext.undoStack.appendToCmdGroup(
                  new CmdChangeNetSignalOfSchematicNetSegment(
                      mPositioningNetPoint2->getNetSegment(), *signal));
            } else {
              QScopedPointer<CmdNetSignalEdit> cmd(new CmdNetSignalEdit(
                  schematic.getProject().getCircuit(),
                  mPositioningNetPoint2->getNetSignalOfNetSegment()));
              cmd->setName(name, false);
              mContext.undoStack.appendToCmdGroup(cmd.take());
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
        abortPositioning(true);
        return false;
      } else {
        return startPositioning(schematic, snap, mPositioningNetPoint2);
      }
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      if (mSubState != SubState::IDLE) {
        abortPositioning(false);
      }
      return false;
    }
  }
}

bool SchematicEditorState_DrawWire::abortPositioning(
    bool showErrMsgBox) noexcept {
  try {
    mCircuit.setHighlightedNetSignal(nullptr);
    mSubState = SubState::IDLE;
    mFixedStartAnchor = nullptr;
    mPositioningNetLine1 = nullptr;
    mPositioningNetLine2 = nullptr;
    mPositioningNetPoint1 = nullptr;
    mPositioningNetPoint2 = nullptr;
    mContext.undoStack.abortCmdGroup();  // can throw
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox)
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

SI_SymbolPin* SchematicEditorState_DrawWire::findSymbolPin(
    Schematic& schematic, const Point& pos) const noexcept {
  QList<SI_SymbolPin*> items = schematic.getPinsAtScenePos(pos);
  for (int i = items.count() - 1; i >= 0; --i) {
    // only choose pins which are connected to a component signal!
    if (!items.at(i)->getComponentSignalInstance()) {
      items.removeAt(i);
    }
  }
  return (items.count() > 0) ? items.first() : nullptr;
}

SI_NetPoint* SchematicEditorState_DrawWire::findNetPoint(
    Schematic& schematic, const Point& pos, SI_NetPoint* except) const
    noexcept {
  QList<SI_NetPoint*> items = schematic.getNetPointsAtScenePos(pos);
  items.removeAll(except);
  return (items.count() > 0) ? items.first() : nullptr;
}

SI_NetLine* SchematicEditorState_DrawWire::findNetLine(Schematic& schematic,
                                                       const Point& pos,
                                                       SI_NetLine* except) const
    noexcept {
  QList<SI_NetLine*> items = schematic.getNetLinesAtScenePos(pos);
  items.removeAll(except);
  return (items.count() > 0) ? items.first() : nullptr;
}

Point SchematicEditorState_DrawWire::updateNetpointPositions(
    Schematic& schematic, bool snap) noexcept {
  // Find anchor under cursor.
  Point pos = mCursorPos.mappedToGrid(getGridInterval());
  if (snap) {
    if (SI_NetPoint* np = findNetPoint(schematic, mCursorPos)) {
      pos = np->getPosition();
    } else if (SI_SymbolPin* pin = findSymbolPin(schematic, mCursorPos)) {
      pos = pin->getPosition();
    }
  }

  mPositioningNetPoint1->setPosition(
      calcMiddlePointPos(mFixedStartAnchor->getPosition(), pos, mWireMode));
  mPositioningNetPoint2->setPosition(pos);
  return pos;
}

void SchematicEditorState_DrawWire::
    updateWireModeActionsCheckedState() noexcept {
  foreach (WireMode key, mWireModeActions.keys()) {
    mWireModeActions.value(key)->setCheckable(key == mWireMode);
    mWireModeActions.value(key)->setChecked(key == mWireMode);
  }
}

Point SchematicEditorState_DrawWire::calcMiddlePointPos(const Point& p1,
                                                        const Point p2,
                                                        WireMode mode) const
    noexcept {
  Point delta = p2 - p1;
  switch (mode) {
    case WireMode_HV:
      return Point(p2.getX(), p1.getY());
    case WireMode_VH:
      return Point(p1.getX(), p2.getY());
    case WireMode_9045:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(
            p2.getX() - delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1),
            p1.getY());
      else
        return Point(
            p1.getX(),
            p2.getY() - delta.getX().abs() * (delta.getY() >= 0 ? 1 : -1));
    case WireMode_4590:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(
            p1.getX() + delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1),
            p2.getY());
      else
        return Point(
            p2.getX(),
            p1.getY() + delta.getX().abs() * (delta.getY() >= 0 ? 1 : -1));
    case WireMode_Straight:
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
}  // namespace project
}  // namespace librepcb
