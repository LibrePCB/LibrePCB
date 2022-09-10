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
#include "packageeditorstate_drawpolygonbase.h"

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../editorcommandset.h"
#include "../../../widgets/angleedit.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../footprintgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/library/pkg/footprint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_DrawPolygonBase::PackageEditorState_DrawPolygonBase(
    Context& context, Mode mode) noexcept
  : PackageEditorState(context),
    mMode(mode),
    mIsUndoCmdActive(false),
    mCurrentPolygon(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sTopPlacement),  // Most important layer
    mLastLineWidth(200000),  // Typical width according library conventions
    mLastAngle(0),
    mLastFill(false),  // Fill is needed very rarely
    mLastGrabArea(false)  // Avoid creating annoying grab areas "by accident"
{
}

PackageEditorState_DrawPolygonBase::
    ~PackageEditorState_DrawPolygonBase() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawPolygonBase::entry() noexcept {
  mContext.graphicsScene.setSelectionArea(QPainterPath());  // clear selection

  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mContext.commandToolBar.addLabel(tr("Layer:"));
  std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
      new GraphicsLayerComboBox());
  layerComboBox->setLayers(getAllowedCircleAndPolygonLayers());
  layerComboBox->setCurrentLayer(mLastLayerName);
  layerComboBox->addAction(
      cmd.layerUp.createAction(layerComboBox.get(), layerComboBox.get(),
                               &GraphicsLayerComboBox::stepDown));
  layerComboBox->addAction(
      cmd.layerDown.createAction(layerComboBox.get(), layerComboBox.get(),
                                 &GraphicsLayerComboBox::stepUp));
  connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &PackageEditorState_DrawPolygonBase::layerComboBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  mContext.commandToolBar.addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(getDefaultLengthUnit(),
                          LengthEditBase::Steps::generic(),
                          "package_editor/draw_polygon/line_width");
  edtLineWidth->setValue(mLastLineWidth);
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, this,
          &PackageEditorState_DrawPolygonBase::lineWidthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtLineWidth));

  if (mMode != Mode::RECT) {
    mContext.commandToolBar.addLabel(tr("Angle:"), 10);
    std::unique_ptr<AngleEdit> edtAngle(new AngleEdit());
    edtAngle->setSingleStep(90.0);  // [°]
    edtAngle->setValue(mLastAngle);
    connect(edtAngle.get(), &AngleEdit::valueChanged, this,
            &PackageEditorState_DrawPolygonBase::angleEditValueChanged);
    mContext.commandToolBar.addWidget(std::move(edtAngle));
  }

  if (mMode != Mode::LINE) {
    std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
    fillCheckBox->setChecked(mLastFill);
    fillCheckBox->addAction(cmd.fillToggle.createAction(
        fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
    connect(fillCheckBox.get(), &QCheckBox::toggled, this,
            &PackageEditorState_DrawPolygonBase::fillCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(fillCheckBox), 10);
  }

  if (mMode != Mode::LINE) {
    std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
    grabAreaCheckBox->setChecked(mLastGrabArea);
    grabAreaCheckBox->addAction(cmd.grabAreaToggle.createAction(
        grabAreaCheckBox.get(), grabAreaCheckBox.get(), &QCheckBox::toggle));
    connect(
        grabAreaCheckBox.get(), &QCheckBox::toggled, this,
        &PackageEditorState_DrawPolygonBase::grabAreaCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(grabAreaCheckBox));
  }

  mLastScenePos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  updateCursorPosition(0);

  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawPolygonBase::exit() noexcept {
  if (!abort()) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  mContext.graphicsView.setSceneCursor(tl::nullopt);
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_DrawPolygonBase::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_DrawPolygonBase::processKeyPressed(
    const QKeyEvent& e) noexcept {
  if (e.key() == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers());
    return true;
  }

  return false;
}

bool PackageEditorState_DrawPolygonBase::processKeyReleased(
    const QKeyEvent& e) noexcept {
  if (e.key() == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers());
    return true;
  }

  return false;
}

bool PackageEditorState_DrawPolygonBase::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = Point::fromPx(e.scenePos());
  updateCursorPosition(e.modifiers());
  return true;
}

bool PackageEditorState_DrawPolygonBase::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = Point::fromPx(e.scenePos());
  if (mIsUndoCmdActive) {
    return addNextSegment();
  } else {
    return start();
  }
}

bool PackageEditorState_DrawPolygonBase::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  // Handle like a single click.
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool PackageEditorState_DrawPolygonBase::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    return abort();
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawPolygonBase::start() noexcept {
  try {
    // Create path.
    Path path;
    for (int i = 0; i < ((mMode == Mode::RECT) ? 5 : 2); ++i) {
      path.addVertex(mCursorPos, (i == 0) ? mLastAngle : Angle::deg0());
    }

    // Add polygon.
    mContext.undoStack.beginCmdGroup(tr("Add footprint polygon"));
    mIsUndoCmdActive = true;
    mCurrentPolygon = std::make_shared<Polygon>(Uuid::createRandom(),
                                                mLastLayerName, mLastLineWidth,
                                                mLastFill, mLastGrabArea, path);
    mContext.undoStack.appendToCmdGroup(new CmdPolygonInsert(
        mContext.currentFootprint->getPolygons(), mCurrentPolygon));
    mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentPolygon);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    abort(false);
    return false;
  }
}

bool PackageEditorState_DrawPolygonBase::abort(bool showErrMsgBox) noexcept {
  try {
    if (mCurrentGraphicsItem) {
      mCurrentGraphicsItem->setSelected(false);
      mCurrentGraphicsItem.reset();
    }
    mEditCmd.reset();
    mCurrentPolygon.reset();
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    }
    return false;
  }
}

bool PackageEditorState_DrawPolygonBase::addNextSegment() noexcept {
  try {
    // If no line was drawn, abort now.
    QVector<Vertex> vertices = mCurrentPolygon->getPath().getVertices();
    bool isEmpty = false;
    if (mMode == Mode::RECT) {
      // Take rect size into account.
      Point size =
          vertices[vertices.count() - 3].getPos() - vertices[0].getPos();
      isEmpty = (size.getX() == 0) || (size.getY() == 0);
    } else {
      // Only take the last line segment into account.
      isEmpty = (vertices[vertices.count() - 1].getPos() ==
                 vertices[vertices.count() - 2].getPos());
    }
    if (isEmpty) {
      return abort();
    }

    // Commit current polygon segment.
    mEditCmd->setPath(Path(vertices), true);
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the polygon is completed, abort now.
    if ((mMode == Mode::RECT) ||
        (vertices.first().getPos() == vertices.last().getPos())) {
      return abort();
    }

    // Add next polygon segment.
    mContext.undoStack.beginCmdGroup(tr("Add footprint polygon"));
    mIsUndoCmdActive = true;
    mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    vertices.last().setAngle(mLastAngle);
    vertices.append(Vertex(mCursorPos, Angle::deg0()));
    mEditCmd->setPath(Path(vertices), true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_DrawPolygonBase::updateCursorPosition(
    Qt::KeyboardModifiers modifiers) noexcept {
  mCursorPos = mLastScenePos;
  if (!modifiers.testFlag(Qt::ShiftModifier)) {
    mCursorPos.mapToGrid(getGridInterval());
  }
  mContext.graphicsView.setSceneCursor(
      std::make_pair(mCursorPos, GraphicsView::CursorOption::Cross));

  if (mCurrentPolygon && mEditCmd) {
    updatePolygonPath();
  }
}

void PackageEditorState_DrawPolygonBase::updatePolygonPath() noexcept {
  QVector<Vertex> vertices = mCurrentPolygon->getPath().getVertices();
  int count = vertices.count();
  if (mMode == Mode::RECT) {
    Q_ASSERT(count >= 5);
    vertices[count - 4].setPos(
        Point(mCursorPos.getX(), vertices[count - 5].getPos().getY()));
    vertices[count - 3].setPos(mCursorPos);
    vertices[count - 2].setPos(
        Point(vertices[count - 5].getPos().getX(), mCursorPos.getY()));
  } else {
    Q_ASSERT(count >= 2);
    vertices[count - 1].setPos(mCursorPos);
  }
  mEditCmd->setPath(Path(vertices), true);
}

void PackageEditorState_DrawPolygonBase::layerComboBoxValueChanged(
    const GraphicsLayerName& layerName) noexcept {
  mLastLayerName = layerName;
  if (mEditCmd) {
    mEditCmd->setLayerName(mLastLayerName, true);
  }
}

void PackageEditorState_DrawPolygonBase::lineWidthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastLineWidth = value;
  if (mEditCmd) {
    mEditCmd->setLineWidth(mLastLineWidth, true);
  }
}

void PackageEditorState_DrawPolygonBase::angleEditValueChanged(
    const Angle& value) noexcept {
  mLastAngle = value;
  if (mCurrentPolygon && mEditCmd) {
    Path path = mCurrentPolygon->getPath();
    if (path.getVertices().count() > 1) {
      path.getVertices()[path.getVertices().count() - 2].setAngle(mLastAngle);
      mEditCmd->setPath(path, true);
    }
  }
}

void PackageEditorState_DrawPolygonBase::fillCheckBoxCheckedChanged(
    bool checked) noexcept {
  mLastFill = checked;
  if (mEditCmd) {
    mEditCmd->setIsFilled(mLastFill, true);
  }
}

void PackageEditorState_DrawPolygonBase::grabAreaCheckBoxCheckedChanged(
    bool checked) noexcept {
  mLastGrabArea = checked;
  if (mEditCmd) {
    mEditCmd->setIsGrabArea(mLastGrabArea, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
