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
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmdchangenetsignalofschematicnetsegment.h"
#include "../../cmd/cmdcombineschematicnetsegments.h"
#include "../../cmd/cmdcompsiginstsetnetsignal.h"
#include "../../cmd/cmdnetclassadd.h"
#include "../../cmd/cmdnetsignaladd.h"
#include "../../cmd/cmdnetsignaledit.h"
#include "../../cmd/cmdschematicnetsegmentadd.h"
#include "../../cmd/cmdschematicnetsegmentaddelements.h"
#include "../../cmd/cmdschematicnetsegmentremoveelements.h"
#include "../../projecteditor.h"
#include "../graphicsitems/sgi_netline.h"
#include "../graphicsitems/sgi_netpoint.h"
#include "../graphicsitems/sgi_symbolpin.h"
#include "../schematiceditor.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
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

  // Add wire mode actions to the "command" toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mWireModeActionGroup = new QActionGroup(&mContext.commandToolBar);
  QAction* aWireModeHV = cmd.wireModeHV.createAction(
      mWireModeActionGroup, this, [this]() { wireModeChanged(WireMode::HV); });
  aWireModeHV->setCheckable(true);
  aWireModeHV->setChecked(mCurrentWireMode == WireMode::HV);
  aWireModeHV->setActionGroup(mWireModeActionGroup);
  QAction* aWireModeVH = cmd.wireModeVH.createAction(
      mWireModeActionGroup, this, [this]() { wireModeChanged(WireMode::VH); });
  aWireModeVH->setCheckable(true);
  aWireModeVH->setChecked(mCurrentWireMode == WireMode::VH);
  aWireModeVH->setActionGroup(mWireModeActionGroup);
  QAction* aWireMode9045 = cmd.wireMode9045.createAction(
      mWireModeActionGroup, this,
      [this]() { wireModeChanged(WireMode::Deg9045); });
  aWireMode9045->setCheckable(true);
  aWireMode9045->setChecked(mCurrentWireMode == WireMode::Deg9045);
  aWireMode9045->setActionGroup(mWireModeActionGroup);
  QAction* aWireMode4590 = cmd.wireMode4590.createAction(
      mWireModeActionGroup, this,
      [this]() { wireModeChanged(WireMode::Deg4590); });
  aWireMode4590->setCheckable(true);
  aWireMode4590->setChecked(mCurrentWireMode == WireMode::Deg4590);
  aWireMode4590->setActionGroup(mWireModeActionGroup);
  QAction* aWireModeStraight = cmd.wireModeStraight.createAction(
      mWireModeActionGroup, this,
      [this]() { wireModeChanged(WireMode::Straight); });
  aWireModeStraight->setCheckable(true);
  aWireModeStraight->setChecked(mCurrentWireMode == WireMode::Straight);
  aWireModeStraight->setActionGroup(mWireModeActionGroup);
  mContext.commandToolBar.addActionGroup(
      std::unique_ptr<QActionGroup>(mWireModeActionGroup));
  mContext.commandToolBar.addSeparator();

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_DrawWire::exit() noexcept {
  // abort the currently active command
  if (mSubState != SubState::IDLE) {
    abortPositioning(true);
  }

  // Remove actions / widgets from the "command" toolbar
  mContext.commandToolBar.clear();

  mContext.editorGraphicsView.unsetCursor();
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
  switch (e.key()) {
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
    const QKeyEvent& e) noexcept {
  switch (e.key()) {
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
    QGraphicsSceneMouseEvent& e) noexcept {
  mCursorPos = Point::fromPx(e.scenePos());

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);
    updateNetpointPositions(snap);
    return true;
  }

  return false;
}

bool SchematicEditorState_DrawWire::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mCursorPos = Point::fromPx(e.scenePos());
  bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);

  if (mSubState == SubState::IDLE) {
    // start adding netpoints/netlines
    return startPositioning(*scene, snap);
  } else if (mSubState == SubState::POSITIONING_NETPOINT) {
    // fix the current point and add a new point + line
    return addNextNetPoint(*scene, snap);
  }

  return false;
}

bool SchematicEditorState_DrawWire::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  mCursorPos = Point::fromPx(e.scenePos());
  bool snap = !e.modifiers().testFlag(Qt::ShiftModifier);

  if (mSubState == SubState::POSITIONING_NETPOINT) {
    // fix the current point and add a new point + line
    return addNextNetPoint(*scene, snap);
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
    // Only switch to next wire mode if cursor was not moved during click.
    if ((mWireModeActionGroup) &&
        (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton))) {
      int index = mWireModeActionGroup->actions().indexOf(
          mWireModeActionGroup->checkedAction());
      index = (index + 1) % mWireModeActionGroup->actions().count();
      QAction* newAction = mWireModeActionGroup->actions().value(index);
      Q_ASSERT(newAction);
      newAction->trigger();
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
    SchematicGraphicsScene& scene, bool snap,
    SI_NetPoint* fixedPoint) noexcept {
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
      std::shared_ptr<QGraphicsItem> item = findItem(mCursorPos);
      if (fixedPoint) {
        mFixedStartAnchor = fixedPoint;
        netsegment = &fixedPoint->getNetSegment();
        pos = fixedPoint->getPosition();
      } else if (auto netpoint =
                     std::dynamic_pointer_cast<SGI_NetPoint>(item)) {
        mFixedStartAnchor = &netpoint->getNetPoint();
        netsegment = &netpoint->getNetPoint().getNetSegment();
        pos = netpoint->getNetPoint().getPosition();
      } else if (auto pin = std::dynamic_pointer_cast<SGI_SymbolPin>(item)) {
        mFixedStartAnchor = &pin->getPin();
        netsegment = pin->getPin().getNetSegmentOfLines();
        netsignal = pin->getPin().getCompSigInstNetSignal();
        pos = pin->getPin().getPosition();
        if (ComponentSignalInstance* sig =
                pin->getPin().getComponentSignalInstance()) {
          QString name = sig->getForcedNetSignalName();
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
      } else if (auto netline = std::dynamic_pointer_cast<SGI_NetLine>(item)) {
        // split netline
        netsegment = &netline->getNetLine().getNetSegment();
        std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
            new CmdSchematicNetSegmentAddElements(*netsegment));
        mFixedStartAnchor = cmdAdd->addNetPoint(Toolbox::nearestPointOnLine(
            pos, netline->getNetLine().getStartPoint().getPosition(),
            netline->getNetLine().getEndPoint().getPosition()));
        cmdAdd->addNetLine(*mFixedStartAnchor,
                           netline->getNetLine().getStartPoint());
        cmdAdd->addNetLine(*mFixedStartAnchor,
                           netline->getNetLine().getEndPoint());
        mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
        std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
            new CmdSchematicNetSegmentRemoveElements(*netsegment));
        cmdRemove->removeNetLine(netline->getNetLine());
        mContext.undoStack.appendToCmdGroup(cmdRemove.release());  // can throw
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
          new CmdSchematicNetSegmentAdd(scene.getSchematic(), *netsignal);
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
    updateNetpointPositions(snap);

    // Highlight all elements of the current netsignal.
    mContext.projectEditor.setHighlightedNetSignals(
        {&netsegment->getNetSignal()});

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    if (mSubState != SubState::IDLE) {
      abortPositioning(false);
    }
    return false;
  }
}

bool SchematicEditorState_DrawWire::addNextNetPoint(
    SchematicGraphicsScene& scene, bool snap) noexcept {
  Q_ASSERT(mSubState == SubState::POSITIONING_NETPOINT);

  // Snap to the item under the cursor and make sure the lines are up to date.
  Point pos = updateNetpointPositions(snap);

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
        if (auto netpoint = std::dynamic_pointer_cast<SGI_NetPoint>(item)) {
          otherAnchor = &netpoint->getNetPoint();
          otherNetSegment = &netpoint->getNetPoint().getNetSegment();
        } else if (auto pin = std::dynamic_pointer_cast<SGI_SymbolPin>(item)) {
          otherAnchor = &pin->getPin();
          otherNetSegment = pin->getPin().getNetSegmentOfLines();
          // connect pin if needed
          if (!otherNetSegment) {
            Q_ASSERT(pin->getPin().getComponentSignalInstance());
            mContext.undoStack.appendToCmdGroup(new CmdCompSigInstSetNetSignal(
                *pin->getPin().getComponentSignalInstance(),
                &mPositioningNetPoint2->getNetSignalOfNetSegment()));
            otherForcedNetName = pin->getPin()
                                     .getComponentSignalInstance()
                                     ->getForcedNetSignalName();
          }
        } else if (auto netline =
                       std::dynamic_pointer_cast<SGI_NetLine>(item)) {
          // split netline
          otherNetSegment = &netline->getNetLine().getNetSegment();
          std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
              new CmdSchematicNetSegmentAddElements(*otherNetSegment));
          otherAnchor = cmdAdd->addNetPoint(pos);
          cmdAdd->addNetLine(*otherAnchor,
                             netline->getNetLine().getStartPoint());
          cmdAdd->addNetLine(*otherAnchor, netline->getNetLine().getEndPoint());
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
          std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
              new CmdSchematicNetSegmentRemoveElements(*otherNetSegment));
          cmdRemove->removeNetLine(netline->getNetLine());
          mContext.undoStack.appendToCmdGroup(
              cmdRemove.release());  // can throw
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
          cmdAdd->addNetLine(*otherAnchor,
                             mPositioningNetLine2->getStartPoint());
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
            NetSignal* signal = mCircuit.getNetSignalByName(*name);
            if (signal) {
              mContext.undoStack.appendToCmdGroup(
                  new CmdChangeNetSignalOfSchematicNetSegment(
                      mPositioningNetPoint2->getNetSegment(), *signal));
            } else {
              std::unique_ptr<CmdNetSignalEdit> cmd(new CmdNetSignalEdit(
                  mCircuit, mPositioningNetPoint2->getNetSignalOfNetSegment()));
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
        return startPositioning(scene, snap, mPositioningNetPoint2);
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
    mContext.projectEditor.clearHighlightedNetSignals();
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

std::shared_ptr<QGraphicsItem> SchematicEditorState_DrawWire::findItem(
    const Point& pos,
    const QVector<std::shared_ptr<QGraphicsItem>>& except) noexcept {
  // Only find pins which are connected to a component signal!
  return findItemAtPos<QGraphicsItem>(
      pos,
      FindFlag::NetPoints | FindFlag::NetLines |
          FindFlag::SymbolPinsWithComponentSignal |
          FindFlag::AcceptNearestWithinGrid,
      except);
}

Point SchematicEditorState_DrawWire::updateNetpointPositions(
    bool snap) noexcept {
  // Find anchor under cursor.
  Point pos = mCursorPos.mappedToGrid(getGridInterval());
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (snap && scene) {
    std::shared_ptr<QGraphicsItem> item =
        findItem(mCursorPos,
                 {
                     scene->getNetPoints().value(mPositioningNetPoint1),
                     scene->getNetPoints().value(mPositioningNetPoint2),
                     scene->getNetLines().value(mPositioningNetLine1),
                     scene->getNetLines().value(mPositioningNetLine2),
                 });
    if (auto netPoint = std::dynamic_pointer_cast<SGI_NetPoint>(item)) {
      pos = netPoint->getNetPoint().getPosition();
    } else if (auto pin = std::dynamic_pointer_cast<SGI_SymbolPin>(item)) {
      pos = pin->getPin().getPosition();
    } else if (auto netline = std::dynamic_pointer_cast<SGI_NetLine>(item)) {
      pos = Toolbox::nearestPointOnLine(
          pos, netline->getNetLine().getStartPoint().getPosition(),
          netline->getNetLine().getEndPoint().getPosition());
    } else if (item) {
      qCritical() << "Found item below cursor, but it has an unexpected type!";
    }
  }

  mPositioningNetPoint1->setPosition(calcMiddlePointPos(
      mFixedStartAnchor->getPosition(), pos, mCurrentWireMode));
  mPositioningNetPoint2->setPosition(pos);
  return pos;
}

void SchematicEditorState_DrawWire::wireModeChanged(WireMode mode) noexcept {
  mCurrentWireMode = mode;
  if (mSubState == SubState::POSITIONING_NETPOINT) {
    updateNetpointPositions(true);
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
