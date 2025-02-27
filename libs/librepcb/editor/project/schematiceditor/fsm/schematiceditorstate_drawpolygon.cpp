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
#include "schematiceditorstate_drawpolygon.h"

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../cmd/cmdschematicpolygonadd.h"
#include "../schematiceditor.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditorState_DrawPolygon::SchematicEditorState_DrawPolygon(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mIsUndoCmdActive(false),
    mLastSegmentPos(),
    mCurrentProperties(Uuid::createRandom(),  // UUID is not relevant here
                       Layer::schematicGuide(),  // Layer
                       UnsignedLength(300000),  // Line width
                       false,  // Is filled
                       false,  // Is grab area
                       Path()  // Path is not relevant here
                       ),
    mCurrentPolygon(nullptr),
    mCurrentPolygonEditCmd(nullptr) {
}

SchematicEditorState_DrawPolygon::~SchematicEditorState_DrawPolygon() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_DrawPolygon::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmSetTool(SchematicEditorFsmAdapter::Tool::Polygon, this);

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the layers combobox to the toolbar
  /*mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(getAllowedGeometryLayers());
  layerComboBox->setCurrentLayer(mLastPolygonProperties.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, this,
          &SchematicEditorState_DrawPolygon::layerComboBoxLayerChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  // Add the width edit to the toolbar
  mContext.commandToolBar.addLabel(tr("Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> widthEdit(new UnsignedLengthEdit());
  widthEdit->setValue(mLastPolygonProperties.getLineWidth());
  widthEdit->addAction(cmd.lineWidthIncrease.createAction(
      widthEdit.get(), widthEdit.get(), &UnsignedLengthEdit::stepUp));
  widthEdit->addAction(cmd.lineWidthDecrease.createAction(
      widthEdit.get(), widthEdit.get(), &UnsignedLengthEdit::stepDown));
  connect(widthEdit.get(), &UnsignedLengthEdit::valueChanged, this,
          &SchematicEditorState_DrawPolygon::widthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(widthEdit));

  // Add the filled checkbox to the toolbar
  mContext.commandToolBar.addLabel(tr("Filled:"), 10);
  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox());
  fillCheckBox->setChecked(mLastPolygonProperties.isFilled());
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  connect(fillCheckBox.get(), &QCheckBox::toggled, this,
          &SchematicEditorState_DrawPolygon::filledCheckBoxCheckedChanged);
  mContext.commandToolBar.addWidget(std::move(fillCheckBox));*/

  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_DrawPolygon::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetTool(SchematicEditorFsmAdapter::Tool::None, this);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_DrawPolygon::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    // Just finish the current polygon, not exiting the whole tool.
    return abortCommand(true);
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool SchematicEditorState_DrawPolygon::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool SchematicEditorState_DrawPolygon::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPolygon(pos);
  }
  return true;
}

bool SchematicEditorState_DrawPolygon::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditorState_DrawPolygon::processSwitchToSchematicPage(
    int index) noexcept {
  // Allow switching to an existing schematic if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void SchematicEditorState_DrawPolygon::setLayer(const Layer& layer) noexcept {
  if (layer != mCurrentProperties.getLayer()) {
    mCurrentProperties.setLayer(layer);
    emit layerChanged(mCurrentProperties.getLayer());
  }

  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLayer(mCurrentProperties.getLayer(), true);
  }
}

void SchematicEditorState_DrawPolygon::setLineWidth(
    const UnsignedLength& width) noexcept {
  if (width != mCurrentProperties.getLineWidth()) {
    mCurrentProperties.setLineWidth(width);
    emit lineWidthChanged(mCurrentProperties.getLineWidth());
  }

  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLineWidth(mCurrentProperties.getLineWidth(),
                                         true);
  }
}

void SchematicEditorState_DrawPolygon::setFilled(bool filled) noexcept {
  if (filled != mCurrentProperties.isFilled()) {
    mCurrentProperties.setIsFilled(filled);
    emit filledChanged(mCurrentProperties.isFilled());
  }

  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setIsFilled(mCurrentProperties.isFilled(), true);
    mCurrentPolygonEditCmd->setIsGrabArea(mCurrentProperties.isFilled(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_DrawPolygon::startAddPolygon(
    const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw schematic polygon"));
    mIsUndoCmdActive = true;

    // Add polygon with two vertices
    mCurrentProperties.setPath(Path({Vertex(pos), Vertex(pos)}));
    mCurrentPolygon = new SI_Polygon(
        *schematic, Polygon(Uuid::createRandom(), mCurrentProperties));
    mContext.undoStack.appendToCmdGroup(
        new CmdSchematicPolygonAdd(*mCurrentPolygon));

    // Start undo command
    mCurrentPolygonEditCmd.reset(
        new CmdPolygonEdit(mCurrentPolygon->getPolygon()));
    mLastSegmentPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool SchematicEditorState_DrawPolygon::addSegment(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  // Abort if no segment drawn
  if (pos == mLastSegmentPos) {
    abortCommand(true);
    return false;
  }

  try {
    // Finish undo command to allow reverting segment by segment
    if (mCurrentPolygonEditCmd) {
      mContext.undoStack.appendToCmdGroup(mCurrentPolygonEditCmd.release());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the polygon is now closed, abort now
    if (mCurrentPolygon->getPolygon().getPath().isClosed()) {
      abortCommand(true);
      return true;
    }

    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw schematic polygon"));
    mIsUndoCmdActive = true;
    mCurrentPolygonEditCmd.reset(
        new CmdPolygonEdit(mCurrentPolygon->getPolygon()));

    // Add new vertex
    Path newPath = mCurrentPolygon->getPolygon().getPath();
    newPath.addVertex(pos, Angle::deg0());
    mCurrentPolygonEditCmd->setPath(newPath, true);
    mLastSegmentPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool SchematicEditorState_DrawPolygon::updateLastVertexPosition(
    const Point& pos) noexcept {
  if (mCurrentPolygonEditCmd) {
    Path newPath = mCurrentPolygon->getPolygon().getPath();
    newPath.getVertices().last().setPos(pos);
    mCurrentPolygonEditCmd->setPath(newPath, true);
    return true;
  } else {
    return false;
  }
}

bool SchematicEditorState_DrawPolygon::abortCommand(
    bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentPolygonEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentPolygon = nullptr;
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
