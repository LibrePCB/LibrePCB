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
#include "boardeditorstate_drawzone.h"

#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../cmd/cmdboardzoneadd.h"
#include "../../cmd/cmdboardzoneedit.h"
#include "../boardeditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_zone.h>
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

BoardEditorState_DrawZone::BoardEditorState_DrawZone(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mLastLayer(&Layer::topCopper()),
    mLastRules(Zone::Rule::All),
    mLastVertexPos(),
    mCurrentZone(nullptr) {
}

BoardEditorState_DrawZone::~BoardEditorState_DrawZone() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawZone::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  Board* board = getActiveBoard();
  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the layer combobox to the toolbar.
  mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  if (board) {
    layerComboBox->setLayers(board->getCopperLayers());
  }
  layerComboBox->setCurrentLayer(*mLastLayer);
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, this,
          [this](const Layer& layer) {
            mLastLayer = &layer;
            if (mCurrentZoneEditCmd) {
              mCurrentZoneEditCmd->setLayers({mLastLayer}, true);
            }
            makeLayerVisible(layer.getThemeColor());
          });
  mContext.commandToolBar.addWidget(std::move(layerComboBox));
  mContext.commandToolBar.addSeparator();

  // Add the "no copper" checkbox to the toolbar.
  std::unique_ptr<QCheckBox> cbxNoCopper(new QCheckBox(tr("No Copper")));
  cbxNoCopper->setChecked(mLastRules.testFlag(Zone::Rule::NoCopper));
  connect(cbxNoCopper.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoCopper, checked);
    if (mCurrentZoneEditCmd) {
      mCurrentZoneEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoCopper));

  // Add the "no planes" checkbox to the toolbar.
  std::unique_ptr<QCheckBox> cbxNoPlanes(new QCheckBox(tr("No Planes")));
  cbxNoPlanes->setChecked(mLastRules.testFlag(Zone::Rule::NoPlanes));
  connect(cbxNoPlanes.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoPlanes, checked);
    if (mCurrentZoneEditCmd) {
      mCurrentZoneEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoPlanes));

  // Add the "no exposure" checkbox to the toolbar.
  std::unique_ptr<QCheckBox> cbxNoExposure(new QCheckBox(tr("No Exposure")));
  cbxNoExposure->setChecked(mLastRules.testFlag(Zone::Rule::NoExposure));
  connect(cbxNoExposure.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoExposure, checked);
    if (mCurrentZoneEditCmd) {
      mCurrentZoneEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoExposure));

  // Add the "no devices" checkbox to the toolbar.
  std::unique_ptr<QCheckBox> cbxNoDevices(new QCheckBox(tr("No Devices")));
  cbxNoDevices->setChecked(mLastRules.testFlag(Zone::Rule::NoDevices));
  connect(cbxNoDevices.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoDevices, checked);
    if (mCurrentZoneEditCmd) {
      mCurrentZoneEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoDevices));

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawZone::exit() noexcept {
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

bool BoardEditorState_DrawZone::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    // Just finish the current zone, not exiting the whole tool.
    return abortCommand(true);
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawZone::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool BoardEditorState_DrawZone::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddZone(pos);
  }
  return true;
}

bool BoardEditorState_DrawZone::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawZone::processSwitchToBoard(int index) noexcept {
  // Allow switching to an existing board if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawZone::startAddZone(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board zone"));
    mIsUndoCmdActive = true;

    // Add zone with two vertices
    Path path({Vertex(pos), Vertex(pos)});
    mCurrentZone = new BI_Zone(*board,
                               BoardZoneData(Uuid::createRandom(), {mLastLayer},
                                             mLastRules, path, false));
    mContext.undoStack.appendToCmdGroup(new CmdBoardZoneAdd(*mCurrentZone));

    // Start undo command
    mCurrentZoneEditCmd.reset(new CmdBoardZoneEdit(*mCurrentZone));
    mLastVertexPos = pos;
    makeLayerVisible(Theme::Color::sBoardZones);
    makeLayerVisible(mLastLayer->getThemeColor());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawZone::addSegment(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  // Abort if no segment drawn
  if (pos == mLastVertexPos) {
    abortCommand(true);
    return false;
  }

  // Abort if the path has been closed.
  Path path = mCurrentZone->getData().getOutline();
  if (path.isClosed()) {
    abortCommand(true);
    return false;
  }

  try {
    // If the zone has more than 2 vertices, start a new undo command
    if (path.getVertices().count() > 2) {
      if (mCurrentZoneEditCmd) {
        mContext.undoStack.appendToCmdGroup(mCurrentZoneEditCmd.take());
      }
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;

      // Start a new undo command
      mContext.undoStack.beginCmdGroup(tr("Draw board zone"));
      mIsUndoCmdActive = true;
      mCurrentZoneEditCmd.reset(new CmdBoardZoneEdit(*mCurrentZone));
    }

    // Add new vertex
    path.addVertex(pos, Angle::deg0());
    if (mCurrentZoneEditCmd) {
      mCurrentZoneEditCmd->setOutline(path, true);
    }
    mLastVertexPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawZone::updateLastVertexPosition(
    const Point& pos) noexcept {
  if (mCurrentZoneEditCmd) {
    Path newPath = mCurrentZone->getData().getOutline();
    newPath.getVertices().last().setPos(pos);
    mCurrentZoneEditCmd->setOutline(Path(newPath), true);
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_DrawZone::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentZoneEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentZone = nullptr;
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
