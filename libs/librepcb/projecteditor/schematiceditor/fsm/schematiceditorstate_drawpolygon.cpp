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

#include "../schematiceditor.h"
#include "../ui_schematiceditor.h"

#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/common/widgets/unsignedlengthedit.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicpolygonadd.h>
#include <librepcb/project/schematics/items/si_polygon.h>
#include <librepcb/project/schematics/schematic.h>

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

SchematicEditorState_DrawPolygon::SchematicEditorState_DrawPolygon(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mIsUndoCmdActive(false),
    mLastPolygonProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        GraphicsLayerName(GraphicsLayer::sSchematicGuide),  // Layer
        UnsignedLength(300000),  // Line width
        false,  // Is filled
        false,  // Is grab area
        Path()  // Path is not relevant here
        ),
    mLastSegmentPos(),
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

  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  // Clear schematic selection because selection does not make sense in this
  // state
  schematic->clearSelection();

  // Add the "Layer:" label to the toolbar
  mLayerLabel.reset(new QLabel(tr("Layer:")));
  mLayerLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mLayerLabel.data());

  // Add the layers combobox to the toolbar
  mLayerComboBox.reset(new GraphicsLayerComboBox());
  mLayerComboBox->setLayers(getAllowedGeometryLayers());
  mLayerComboBox->setCurrentLayer(mLastPolygonProperties.getLayerName());
  mContext.editorUi.commandToolbar->addWidget(mLayerComboBox.data());
  connect(mLayerComboBox.data(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &SchematicEditorState_DrawPolygon::layerComboBoxLayerChanged);

  // Add the "Width:" label to the toolbar
  mWidthLabel.reset(new QLabel(tr("Width:")));
  mWidthLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mWidthLabel.data());

  // Add the widths combobox to the toolbar
  mWidthEdit.reset(new UnsignedLengthEdit());
  mWidthEdit->setValue(mLastPolygonProperties.getLineWidth());
  mContext.editorUi.commandToolbar->addWidget(mWidthEdit.data());
  connect(mWidthEdit.data(), &UnsignedLengthEdit::valueChanged, this,
          &SchematicEditorState_DrawPolygon::widthEditValueChanged);

  // Add the "Filled:" label to the toolbar
  mFillLabel.reset(new QLabel(tr("Filled:")));
  mFillLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mFillLabel.data());

  // Add the filled checkbox to the toolbar
  mFillCheckBox.reset(new QCheckBox());
  mFillCheckBox->setChecked(mLastPolygonProperties.isFilled());
  mContext.editorUi.commandToolbar->addWidget(mFillCheckBox.data());
  connect(mFillCheckBox.data(), &QCheckBox::toggled, this,
          &SchematicEditorState_DrawPolygon::filledCheckBoxCheckedChanged);

  // Change the cursor
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool SchematicEditorState_DrawPolygon::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mFillCheckBox.reset();
  mFillLabel.reset();
  mWidthEdit.reset();
  mWidthLabel.reset();
  mLayerComboBox.reset();
  mLayerLabel.reset();
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();

  // Reset the cursor
  mContext.editorGraphicsView.setCursor(Qt::ArrowCursor);

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
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPolygon(*schematic, pos);
  }
  return true;
}

bool SchematicEditorState_DrawPolygon::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditorState_DrawPolygon::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processAbortCommand();
}

bool SchematicEditorState_DrawPolygon::processSwitchToSchematicPage(
    int index) noexcept {
  // Allow switching to an existing schematic if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_DrawPolygon::startAddPolygon(
    Schematic& schematic, const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw schematic polygon"));
    mIsUndoCmdActive = true;

    // Add polygon with two vertices
    mLastPolygonProperties.setPath(Path({Vertex(pos), Vertex(pos)}));
    mCurrentPolygon = new SI_Polygon(
        schematic, Polygon(Uuid::createRandom(), mLastPolygonProperties));
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
      mContext.undoStack.appendToCmdGroup(mCurrentPolygonEditCmd.take());
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

void SchematicEditorState_DrawPolygon::layerComboBoxLayerChanged(
    const GraphicsLayerName& layerName) noexcept {
  mLastPolygonProperties.setLayerName(layerName);
  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLayerName(mLastPolygonProperties.getLayerName(),
                                         true);
  }
}

void SchematicEditorState_DrawPolygon::widthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastPolygonProperties.setLineWidth(value);
  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLineWidth(mLastPolygonProperties.getLineWidth(),
                                         true);
  }
}

void SchematicEditorState_DrawPolygon::filledCheckBoxCheckedChanged(
    bool checked) noexcept {
  mLastPolygonProperties.setIsFilled(checked);
  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setIsFilled(mLastPolygonProperties.isFilled(),
                                        true);
    mCurrentPolygonEditCmd->setIsGrabArea(mLastPolygonProperties.isFilled(),
                                          true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
