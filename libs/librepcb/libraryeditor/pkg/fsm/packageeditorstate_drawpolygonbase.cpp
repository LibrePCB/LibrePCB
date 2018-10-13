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

#include "../packageeditorwidget.h"

#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_DrawPolygonBase::PackageEditorState_DrawPolygonBase(
    Context& context, Mode mode) noexcept
  : PackageEditorState(context),
    mMode(mode),
    mCurrentPolygon(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sTopPlacement),
    mLastLineWidth(254000),
    mLastAngle(0),
    mLastFill(false),
    mLastGrabArea(mode != Mode::LINE) {
}

PackageEditorState_DrawPolygonBase::
    ~PackageEditorState_DrawPolygonBase() noexcept {
  Q_ASSERT(mEditCmd.isNull());
  Q_ASSERT(mCurrentPolygon == nullptr);
  Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawPolygonBase::entry() noexcept {
  mContext.graphicsScene.setSelectionArea(QPainterPath());  // clear selection
  mContext.graphicsView.setCursor(Qt::CrossCursor);

  // populate command toolbar
  mContext.commandToolBar.addLabel(tr("Layer:"));
  std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
      new GraphicsLayerComboBox());
  layerComboBox->setLayers(
      mContext.layerProvider.getBoardGeometryElementLayers());
  layerComboBox->setCurrentLayer(*mLastLayerName);
  connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &PackageEditorState_DrawPolygonBase::layerComboBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  mContext.commandToolBar.addLabel(tr("Line Width:"), 10);
  std::unique_ptr<QDoubleSpinBox> lineWidthSpinBox(new QDoubleSpinBox());
  lineWidthSpinBox->setMinimum(0);
  lineWidthSpinBox->setMaximum(100);
  lineWidthSpinBox->setSingleStep(0.1);
  lineWidthSpinBox->setDecimals(6);
  lineWidthSpinBox->setValue(mLastLineWidth->toMm());
  connect(lineWidthSpinBox.get(),
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this,
          &PackageEditorState_DrawPolygonBase::lineWidthSpinBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(lineWidthSpinBox));

  if (mMode != Mode::RECT) {
    mContext.commandToolBar.addLabel(tr("Angle:"), 10);
    std::unique_ptr<QDoubleSpinBox> angleSpinBox(new QDoubleSpinBox());
    angleSpinBox->setMinimum(-360);
    angleSpinBox->setMaximum(360);
    angleSpinBox->setSingleStep(30);
    angleSpinBox->setDecimals(6);
    angleSpinBox->setValue(mLastAngle.toDeg());
    connect(angleSpinBox.get(),
            static_cast<void (QDoubleSpinBox::*)(double)>(
                &QDoubleSpinBox::valueChanged),
            this,
            &PackageEditorState_DrawPolygonBase::angleSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(angleSpinBox));
  }

  if (mMode != Mode::LINE) {
    std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
    fillCheckBox->setChecked(mLastFill);
    connect(fillCheckBox.get(), &QCheckBox::toggled, this,
            &PackageEditorState_DrawPolygonBase::fillCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(fillCheckBox));
  }

  if (mMode != Mode::LINE) {
    std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
    grabAreaCheckBox->setChecked(mLastGrabArea);
    connect(
        grabAreaCheckBox.get(), &QCheckBox::toggled, this,
        &PackageEditorState_DrawPolygonBase::grabAreaCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(grabAreaCheckBox));
  }

  return true;
}

bool PackageEditorState_DrawPolygonBase::exit() noexcept {
  if (mCurrentPolygon && (!abort())) {
    return false;
  }

  // cleanup command toolbar
  mContext.commandToolBar.clear();

  mContext.graphicsView.setCursor(Qt::ArrowCursor);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_DrawPolygonBase::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentPolygon) {
    Point currentPos =
        Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    return updateCurrentPosition(currentPos);
  } else {
    return true;
  }
}

bool PackageEditorState_DrawPolygonBase::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mCurrentPolygon) {
    Point startPos = mCurrentPolygon->getPath().getVertices().first().getPos();
    if (currentPos == mSegmentStartPos) {
      return abort();
    } else if ((currentPos == startPos) || (mMode == Mode::RECT)) {
      return addNextSegment(currentPos) && abort();
    } else {
      return addNextSegment(currentPos);
    }
  } else {
    return start(currentPos);
  }
}

bool PackageEditorState_DrawPolygonBase::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(
      e);  // handle like single click
}

bool PackageEditorState_DrawPolygonBase::processAbortCommand() noexcept {
  if (mCurrentPolygon) {
    return abort();
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawPolygonBase::start(const Point& pos) noexcept {
  try {
    // create path
    Path path;
    for (int i = 0; i < ((mMode == Mode::RECT) ? 5 : 2); ++i) {
      path.addVertex(pos, (i == 0) ? mLastAngle : Angle::deg0());
    }

    // add polygon
    mSegmentStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add footprint polygon"));
    mCurrentPolygon.reset(new Polygon(Uuid::createRandom(), mLastLayerName,
                                      mLastLineWidth, mLastFill, mLastGrabArea,
                                      path));
    mContext.undoStack.appendToCmdGroup(new CmdPolygonInsert(
        mContext.currentFootprint->getPolygons(), mCurrentPolygon));
    mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getPolygonGraphicsItem(*mCurrentPolygon);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem = nullptr;
    mEditCmd.reset();
    mCurrentPolygon.reset();
    return false;
  }
}

bool PackageEditorState_DrawPolygonBase::abort() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem = nullptr;
    mEditCmd.reset();
    mCurrentPolygon.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_DrawPolygonBase::addNextSegment(
    const Point& pos) noexcept {
  try {
    // commit current
    updateCurrentPosition(pos);
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();

    // add next
    mSegmentStartPos = pos;
    mContext.undoStack.beginCmdGroup(tr("Add footprint polygon"));
    mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    Path newPath = mCurrentPolygon->getPath();
    newPath.getVertices().last().setAngle(mLastAngle);
    newPath.addVertex(pos, Angle::deg0());
    mEditCmd->setPath(newPath, true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_DrawPolygonBase::updateCurrentPosition(
    const Point& pos) noexcept {
  if ((!mCurrentPolygon) || (!mEditCmd)) return false;
  QVector<Vertex> vertices = mCurrentPolygon->getPath().getVertices();
  int             count    = vertices.count();
  if (mMode == Mode::RECT) {
    Q_ASSERT(count >= 5);
    vertices[count - 4].setPos(
        Point(pos.getX(), vertices[count - 5].getPos().getY()));
    vertices[count - 3].setPos(pos);
    vertices[count - 2].setPos(
        Point(vertices[count - 5].getPos().getX(), pos.getY()));
  } else {
    Q_ASSERT(count >= 2);
    vertices[count - 1].setPos(pos);
  }
  mEditCmd->setPath(Path(vertices), true);
  return true;
}

void PackageEditorState_DrawPolygonBase::layerComboBoxValueChanged(
    const QString& layerName) noexcept {
  if (layerName.isEmpty()) {
    return;
  }
  mLastLayerName = layerName;
  if (mEditCmd) {
    mEditCmd->setLayerName(mLastLayerName, true);
  }
}

void PackageEditorState_DrawPolygonBase::lineWidthSpinBoxValueChanged(
    double value) noexcept {
  mLastLineWidth = Length::fromMm(value);
  if (mEditCmd) {
    mEditCmd->setLineWidth(mLastLineWidth, true);
  }
}

void PackageEditorState_DrawPolygonBase::angleSpinBoxValueChanged(
    double value) noexcept {
  mLastAngle = Angle::fromDeg(value);
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
}  // namespace library
}  // namespace librepcb
