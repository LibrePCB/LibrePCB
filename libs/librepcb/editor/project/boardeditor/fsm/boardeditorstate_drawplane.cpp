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

#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../cmd/cmdboardplaneadd.h"
#include "../../cmd/cmdboardplaneedit.h"
#include "../boardeditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/toolbox.h>

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
    mLastLayer(&Layer::topCopper()),
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

  // Get most used net signal
  if ((!mLastNetSignal) || (!mLastNetSignal->isAddedToCircuit())) {
    mLastNetSignal =
        mContext.project.getCircuit().getNetSignalWithMostElements();
  }
  if (!mLastNetSignal) {
    QMessageBox::warning(&mContext.editor, tr("No net available"),
                         tr("Your circuit doesn't contain any net, please add "
                            "one in the schematic editor first."));
    return false;
  }

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the netsignals combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Net:"), 10);
  std::unique_ptr<QComboBox> netSignalComboBox(new QComboBox());
  netSignalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  netSignalComboBox->setInsertPolicy(QComboBox::NoInsert);
  netSignalComboBox->setEditable(false);
  QList<NetSignal*> netSignals =
      mContext.project.getCircuit().getNetSignals().values();
  Toolbox::sortNumeric(
      netSignals,
      [](const QCollator& cmp, const NetSignal* lhs, const NetSignal* rhs) {
        return cmp(*lhs->getName(), *rhs->getName());
      },
      Qt::CaseInsensitive, false);
  netSignalComboBox->addItem("[" % tr("None") % "]", QString());
  foreach (const NetSignal* netsignal, netSignals) {
    netSignalComboBox->addItem(*netsignal->getName(),
                               netsignal->getUuid().toStr());
  }
  netSignalComboBox->setCurrentText(mLastNetSignal ? *mLastNetSignal->getName()
                                                   : "");
  connect(
      netSignalComboBox.get(), &QComboBox::currentTextChanged,
      [this](const QString& value) {
        setNetSignal(mContext.project.getCircuit().getNetSignalByName(value));
      });
  mContext.commandToolBar.addWidget(std::move(netSignalComboBox));

  // Add the layers combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  QSet<const Layer*> layers;
  if (Board* board = getActiveBoard()) {
    layers = board->getCopperLayers();
  }
  layerComboBox->setLayers(layers);
  layerComboBox->setCurrentLayer(*mLastLayer);
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, this,
          &BoardEditorState_DrawPlane::layerComboBoxLayerChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawPlane::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mContext.commandToolBar.clear();

  mContext.editorGraphicsView.unsetCursor();
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
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPlane(pos);
  }
  return true;
}

bool BoardEditorState_DrawPlane::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawPlane::processSwitchToBoard(int index) noexcept {
  // Allow switching to an existing board if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawPlane::startAddPlane(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board plane"));
    mIsUndoCmdActive = true;

    // Add plane with two vertices
    Path path({Vertex(pos), Vertex(pos)});
    mCurrentPlane = new BI_Plane(*board, Uuid::createRandom(), *mLastLayer,
                                 mLastNetSignal, path);
    mCurrentPlane->setConnectStyle(BI_Plane::ConnectStyle::ThermalRelief);
    mContext.undoStack.appendToCmdGroup(new CmdBoardPlaneAdd(*mCurrentPlane));

    // Start undo command
    mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane));
    mLastVertexPos = pos;
    makeLayerVisible(mLastLayer->getThemeColor());
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
        mContext.undoStack.appendToCmdGroup(mCurrentPlaneEditCmd.release());
      }
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;

      // Start a new undo command
      mContext.undoStack.beginCmdGroup(tr("Draw board plane"));
      mIsUndoCmdActive = true;
      mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane));
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
  mLastNetSignal = netsignal;
  if (mCurrentPlaneEditCmd) {
    mCurrentPlaneEditCmd->setNetSignal(mLastNetSignal);
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
    const Layer& layer) noexcept {
  mLastLayer = &layer;
  if (mCurrentPlaneEditCmd) {
    mCurrentPlaneEditCmd->setLayer(*mLastLayer, true);
    makeLayerVisible(mLastLayer->getThemeColor());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
