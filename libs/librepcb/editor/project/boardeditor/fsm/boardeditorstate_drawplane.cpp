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
#include "boardeditorstate_drawplane.h"

#include "../../../undostack.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmdboardplaneadd.h"
#include "../../cmd/cmdboardplaneedit.h"
#include "../boardeditor.h"

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_DrawPlane::BoardEditorState_DrawPlane(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mLastNetSignal(nullptr),
    mLastLayerName(GraphicsLayer::sTopCopper),
    mLastVertexPos(),
    mCurrentPlane(nullptr) {
}

BoardEditorState_DrawPlane::~BoardEditorState_DrawPlane() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawPlane::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  Board* board = getActiveBoard();
  if (!board) return false;

  // Get most used net signal
  if (!mLastNetSignal) {
    mLastNetSignal =
        mContext.project.getCircuit().getNetSignalWithMostElements();
  }
  if (!mLastNetSignal) return false;

  // Clear board selection because selection does not make sense in this state
  board->clearSelection();

  // Add the "Signal:" label to the toolbar
  mNetSignalLabel.reset(new QLabel(tr("Signal:")));
  mNetSignalLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mNetSignalLabel.data());

  // Add the netsignals combobox to the toolbar
  mNetSignalComboBox.reset(new QComboBox());
  mNetSignalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mNetSignalComboBox->setInsertPolicy(QComboBox::NoInsert);
  mNetSignalComboBox->setEditable(false);
  foreach (NetSignal* netsignal, mContext.project.getCircuit().getNetSignals())
    mNetSignalComboBox->addItem(*netsignal->getName(),
                                netsignal->getUuid().toStr());
  mNetSignalComboBox->model()->sort(0);
  mNetSignalComboBox->setCurrentText(mLastNetSignal ? *mLastNetSignal->getName()
                                                    : "");
  mContext.editorUi.commandToolbar->addWidget(mNetSignalComboBox.data());
  connect(
      mNetSignalComboBox.data(), &QComboBox::currentTextChanged,
      [this](const QString& value) {
        setNetSignal(mContext.project.getCircuit().getNetSignalByName(value));
      });

  // Add the "Layer:" label to the toolbar
  mLayerLabel.reset(new QLabel(tr("Layer:")));
  mLayerLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mLayerLabel.data());

  // Add the layers combobox to the toolbar
  mLayerComboBox.reset(new GraphicsLayerComboBox());
  QList<GraphicsLayer*> layers;
  foreach (GraphicsLayer* layer, board->getLayerStack().getAllLayers()) {
    if (layer->isCopperLayer() && layer->isEnabled()) {
      layers.append(layer);
    }
  }
  mLayerComboBox->setLayers(layers);
  mLayerComboBox->setCurrentLayer(mLastLayerName);
  mContext.editorUi.commandToolbar->addWidget(mLayerComboBox.data());
  connect(mLayerComboBox.data(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &BoardEditorState_DrawPlane::layerComboBoxLayerChanged);

  // Change the cursor
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool BoardEditorState_DrawPlane::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mLayerComboBox.reset();
  mLayerLabel.reset();
  mNetSignalComboBox.reset();
  mNetSignalLabel.reset();
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();

  // Reset the cursor
  mContext.editorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_DrawPlane::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    // Just finish the current plane, not exiting the whole tool.
    return abortCommand(true);
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawPlane::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool BoardEditorState_DrawPlane::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPlane(*board, pos);
  }
  return true;
}

bool BoardEditorState_DrawPlane::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawPlane::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processAbortCommand();
}

bool BoardEditorState_DrawPlane::processSwitchToBoard(int index) noexcept {
  // Allow switching to an existing board if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawPlane::startAddPlane(Board& board,
                                               const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board plane"));
    mIsUndoCmdActive = true;

    // Add plane with two vertices
    Path path({Vertex(pos), Vertex(pos)});
    mCurrentPlane = new BI_Plane(board, Uuid::createRandom(), mLastLayerName,
                                 *mLastNetSignal, path);
    mContext.undoStack.appendToCmdGroup(new CmdBoardPlaneAdd(*mCurrentPlane));

    // Start undo command
    mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane, false));
    mLastVertexPos = pos;
    makeSelectedLayerVisible();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawPlane::addSegment(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  // Abort if no segment drawn
  if (pos == mLastVertexPos) {
    abortCommand(true);
    return false;
  }

  try {
    // If the plane has more than 2 vertices, start a new undo command
    if (mCurrentPlane->getOutline().getVertices().count() > 2) {
      if (mCurrentPlaneEditCmd) {
        mContext.undoStack.appendToCmdGroup(mCurrentPlaneEditCmd.take());
      }
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;

      // Start a new undo command
      mContext.undoStack.beginCmdGroup(tr("Draw board plane"));
      mIsUndoCmdActive = true;
      mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane, false));
    }

    // Add new vertex
    Path newPath = mCurrentPlane->getOutline();
    newPath.addVertex(pos, Angle::deg0());
    if (mCurrentPlaneEditCmd) {
      mCurrentPlaneEditCmd->setOutline(newPath, true);
    }
    mLastVertexPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawPlane::updateLastVertexPosition(
    const Point& pos) noexcept {
  if (mCurrentPlaneEditCmd) {
    Path newPath = mCurrentPlane->getOutline();
    newPath.getVertices().last().setPos(pos);
    mCurrentPlaneEditCmd->setOutline(newPath, true);
    return true;
  } else {
    return false;
  }
}

void BoardEditorState_DrawPlane::setNetSignal(NetSignal* netsignal) noexcept {
  try {
    if (!netsignal) throw LogicError(__FILE__, __LINE__);
    mLastNetSignal = netsignal;
    if (mCurrentPlaneEditCmd) {
      mCurrentPlaneEditCmd->setNetSignal(*mLastNetSignal);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

bool BoardEditorState_DrawPlane::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentPlaneEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentPlane = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

void BoardEditorState_DrawPlane::layerComboBoxLayerChanged(
    const GraphicsLayerName& layerName) noexcept {
  mLastLayerName = layerName;
  if (mCurrentPlaneEditCmd) {
    mCurrentPlaneEditCmd->setLayerName(mLastLayerName, true);
    makeSelectedLayerVisible();
  }
}

void BoardEditorState_DrawPlane::makeSelectedLayerVisible() noexcept {
  if (mCurrentPlane) {
    Board& board = mCurrentPlane->getBoard();
    GraphicsLayer* layer = board.getLayerStack().getLayer(*mLastLayerName);
    if (layer && layer->isEnabled()) layer->setVisible(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
