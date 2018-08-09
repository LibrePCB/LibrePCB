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
#include "bes_drawtrace.h"

#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../boardeditor.h"
#include "../pns_router/pns_librepcb_iface.h"
#include "ui_boardeditor.h"

#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/circuit/circuit.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BES_DrawTrace::BES_DrawTrace(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView,
                             UndoStack&    undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState_Idle),
    mCurrentLayerName(GraphicsLayer::sTopCopper),
    mCurrentWidth(500000),
    // command toolbar actions / widgets:
    mLayerLabel(nullptr),
    mLayerComboBox(nullptr),
    mWidthLabel(nullptr),
    mWidthComboBox(nullptr) {
}

BES_DrawTrace::~BES_DrawTrace() {
  Q_ASSERT(mSubState == SubState_Idle);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::process(BEE_Base* event) noexcept {
  switch (mSubState) {
    case SubState_Idle:
      return processSubStateIdle(event);
    case SubState_PositioningNetPoint:
      return processSubStatePositioning(event);
    default:
      Q_ASSERT(false);
      return PassToParentState;
  }
}

bool BES_DrawTrace::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  Q_ASSERT(mSubState == SubState_Idle);

  // clear board selection because selection does not make sense in this state
  if (mEditor.getActiveBoard()) mEditor.getActiveBoard()->clearSelection();

  // add the "Layer:" label to the toolbar
  mLayerLabel = new QLabel(tr("Layer:"));
  mLayerLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mLayerLabel);

  // add the layers combobox to the toolbar
  mLayerComboBox = new QComboBox();
  mLayerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mLayerComboBox->setInsertPolicy(QComboBox::NoInsert);
  if (mEditor.getActiveBoard()) {
    foreach (const auto& layer,
             mEditor.getActiveBoard()->getLayerStack().getAllLayers()) {
      if (layer->isCopperLayer() && layer->isEnabled()) {
        mLayerComboBox->addItem(layer->getNameTr(), layer->getName());
      }
    }
  }
  mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(mCurrentLayerName));
  mEditorUi.commandToolbar->addWidget(mLayerComboBox);
  connect(
      mLayerComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BES_DrawTrace::layerComboBoxIndexChanged);

  // add the "Width:" label to the toolbar
  mWidthLabel = new QLabel(tr("Width:"));
  mWidthLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mWidthLabel);

  // add the widths combobox to the toolbar
  mWidthComboBox = new QComboBox();
  mWidthComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mWidthComboBox->setInsertPolicy(QComboBox::NoInsert);
  mWidthComboBox->setEditable(true);
  mWidthComboBox->addItem("0.2");
  mWidthComboBox->addItem("0.3");
  mWidthComboBox->addItem("0.5");
  mWidthComboBox->addItem("0.8");
  mWidthComboBox->addItem("1");
  mWidthComboBox->addItem("1.5");
  mWidthComboBox->addItem("2");
  mWidthComboBox->addItem("2.5");
  mWidthComboBox->addItem("3");
  mWidthComboBox->setCurrentIndex(
      mWidthComboBox->findText(QString::number(mCurrentWidth->toMm())));
  mEditorUi.commandToolbar->addWidget(mWidthComboBox);
  connect(mWidthComboBox, &QComboBox::currentTextChanged, this,
          &BES_DrawTrace::wireWidthComboBoxTextChanged);

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::CrossCursor);

  // PNS
  iface = new PNS::PNS_LIBREPCB_IFACE;
  iface->SetBoard(mEditor.getActiveBoard());
  iface->create_debug_decorator();
  router = new PNS::ROUTER;
  router->SetInterface(iface);
  router->SetMode(PNS::ROUTER_MODE::PNS_MODE_ROUTE_SINGLE);
  router->ClearWorld();
  router->SyncWorld();
  PNS::ROUTING_SETTINGS settings;
  settings.SetShoveVias(false);
  PNS::SIZES_SETTINGS sizes_settings;
  router->LoadSettings(settings);
  router->UpdateSizes(sizes_settings);

  // Start routing?
  // Track *track = get_track(args.selection);
  // if (!track) {
  //    return ToolResponse::end();
  //}
  // auto parent = iface->get_parent(track);
  // wrapper->m_startItem = router->GetWorld()->FindItemByParent(parent,
  // iface->get_net_code(track->net.uuid)); VECTOR2I p0(args.coords.x,
  // args.coords.y); if (!router->StartDragging(p0, wrapper->m_startItem,
  // PNS::DM_ANY))
  //    return ToolResponse::end();

  return true;
}

bool BES_DrawTrace::exit(BEE_Base* event) noexcept {
  Q_UNUSED(event);

  // abort the currently active command
  if (mSubState != SubState_Idle) abortPositioning(true);

  delete router;
  delete iface;

  // Remove actions / widgets from the "command" toolbar
  delete mWidthComboBox;
  mWidthComboBox = nullptr;
  delete mWidthLabel;
  mWidthLabel = nullptr;
  delete mLayerComboBox;
  mLayerComboBox = nullptr;
  delete mLayerLabel;
  mLayerLabel = nullptr;
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::processSubStateIdle(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::GraphicsViewEvent:
      return processIdleSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_DrawTrace::processIdleSceneEvent(
    BEE_Base* event) noexcept {
  QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return PassToParentState;

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Point pos = Point::fromPx(sceneEvent->scenePos());

      switch (sceneEvent->button()) {
        case Qt::LeftButton:
          // start adding netpoints/netlines
          startPositioning(*board, pos);
          return ForceStayInState;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  return PassToParentState;
}

BES_Base::ProcRetVal BES_DrawTrace::processSubStatePositioning(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::AbortCommand:
      abortPositioning(true);
      return ForceStayInState;
    case BEE_Base::GraphicsViewEvent:
      return processPositioningSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_DrawTrace::processPositioningSceneEvent(
    BEE_Base* event) noexcept {
  QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return PassToParentState;

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      VECTOR2I posv(pos.getX().toNm(), pos.getY().toNm());
      switch (sceneEvent->button()) {
        case Qt::LeftButton:
          // fix the current point and add a new point + line

          // PNS
          if (router->FixRoute(posv, nullptr)) {
            // router->StopRouting();
            // imp->canvas_update();
            // state = State::START;
            // imp->get_highlights().clear();
            // imp->update_highlights();
            // update_tip();
            // return ToolResponse();
          }
          // router->Move(wrapper->m_endSnapPoint, wrapper->m_endItem);

          return ForceStayInState;
        case Qt::RightButton:
          return ForceStayInState;
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      switch (sceneEvent->button()) {
        case Qt::RightButton:
          if (sceneEvent->screenPos() ==
              sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
            return ForceStayInState;
          }
          break;
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());

      // PNS
      VECTOR2I posv(pos.getX().toNm(), pos.getY().toNm());
      router->Move(posv, nullptr);

      return ForceStayInState;
    }

    default:
      break;
  }

  return PassToParentState;
}

bool BES_DrawTrace::startPositioning(Board& board, const Point& pos,
                                     BI_NetPoint* /*fixedPoint*/) noexcept {
  Point posOnGrid = pos.mappedToGrid(board.getGridProperties().getInterval());

  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState_Idle);
    mUndoStack.beginCmdGroup(tr("Draw Board Trace"));
    mSubState = SubState_PositioningNetPoint;

    // PNS
    int        tl = PNS::PNS_LIBREPCB_IFACE::layer_to_router(mCurrentLayerName);
    VECTOR2I   posv(pos.getX().toNm(), pos.getY().toNm());
    PNS::ITEM* prioritized[4] = {0};
    PNS::ITEM_SET candidates  = router->QueryHoverItems(posv);

    for (PNS::ITEM* item : candidates.Items()) {
      // auto la =
      // PNS::PNS_LIBREPCB_IFACE::layer_from_router(item->Layers().Start());
      if (item->OfKind(PNS::ITEM::VIA_T | PNS::ITEM::SOLID_T)) {
        if (!prioritized[2]) prioritized[2] = item;
        if (item->Layers().Overlaps(tl)) prioritized[0] = item;
      } else {
        if (!prioritized[3]) prioritized[3] = item;
        if (item->Layers().Overlaps(tl)) prioritized[1] = item;
      }
    }
    PNS::ITEM* rv = NULL;
    for (int i = 0; i < 4; i++) {
      PNS::ITEM* item = prioritized[i];
      qDebug() << item;

      // if (tool->canvas->selection_filter.work_layer_only)
      //    if (item && !item->Layers().Overlaps(tl))
      //        item = NULL;

      if (item) {
        rv = item;
        break;
      }
    }

    if (rv) {
      PNS::PNS_LIBREPCB_IFACE* iface =
          dynamic_cast<PNS::PNS_LIBREPCB_IFACE*>(router->GetInterface());
      NetSignal* sig = iface->get_net_for_code(rv->Net());
      mCircuit.setHighlightedNetSignal(sig);
    }

    // PNS
    PNS::SIZES_SETTINGS sizes(router->Sizes());
    sizes.SetTrackWidth(mCurrentWidth->toNm());
    router->UpdateSizes(sizes);
    PNS::ROUTING_SETTINGS settings(router->Settings());
    settings.SetMode(PNS::RM_Shove);
    router->LoadSettings(settings);
    if (!router->StartRouting(posv, rv, tl)) {
      std::cout << "error " << router->FailureReason() << std::endl;
      return false;
    }

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    if (mSubState != SubState_Idle) {
      abortPositioning(false);
    }
    return false;
  }
}

bool BES_DrawTrace::abortPositioning(bool showErrMsgBox) noexcept {
  try {
    router->StopRouting();

    mCircuit.setHighlightedNetSignal(nullptr);
    mSubState = SubState_Idle;
    mUndoStack.abortCmdGroup();  // can throw
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_DrawTrace::layerComboBoxIndexChanged(int index) noexcept {
  mCurrentLayerName = mLayerComboBox->itemData(index).toString();
  // TODO: add a via to change the layer of the current netline?
}

void BES_DrawTrace::wireWidthComboBoxTextChanged(
    const QString& width) noexcept {
  try {
    mCurrentWidth = Length::fromMm(width);
  } catch (...) {
    return;
  }
  if (mSubState != SubState::SubState_PositioningNetPoint) return;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
