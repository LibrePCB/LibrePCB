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
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../widgets/angleedit.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../footprintgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/library/pkg/footprint.h>
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

PackageEditorState_DrawPolygonBase::PackageEditorState_DrawPolygonBase(
    Context& context, Mode mode) noexcept
  : PackageEditorState(context),
    mMode(mode),
    mIsUndoCmdActive(false),
    mCurrentPolygon(nullptr),
    mCurrentGraphicsItem(nullptr),
    mArcCenter(),
    mArcInSecondState(false),
    mLastLayer(&Layer::topLegend()),  // Most important layer
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
  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mContext.commandToolBar.addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(getAllowedCircleAndPolygonLayers());
  layerComboBox->setCurrentLayer(*mLastLayer);
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, this,
          &PackageEditorState_DrawPolygonBase::layerComboBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  mContext.commandToolBar.addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(getLengthUnit(), LengthEditBase::Steps::generic(),
                          "package_editor/draw_polygon/line_width");
  edtLineWidth->setValue(mLastLineWidth);
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  connect(this, &PackageEditorState_DrawPolygonBase::requestLineWidth,
          edtLineWidth.get(), &UnsignedLengthEdit::setValue);
  connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, this,
          &PackageEditorState_DrawPolygonBase::lineWidthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtLineWidth));

  if ((mMode == Mode::LINE) || (mMode == Mode::POLYGON)) {
    mContext.commandToolBar.addLabel(tr("Arc Angle:"), 10);
    std::unique_ptr<AngleEdit> edtAngle(new AngleEdit());
    edtAngle->setSingleStep(90.0);  // [°]
    edtAngle->setValue(mLastAngle);
    connect(edtAngle.get(), &AngleEdit::valueChanged, this,
            &PackageEditorState_DrawPolygonBase::angleEditValueChanged);
    mContext.commandToolBar.addWidget(std::move(edtAngle));
  }

  if ((mMode == Mode::RECT) || (mMode == Mode::POLYGON)) {
    std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
    fillCheckBox->setChecked(mLastFill);
    fillCheckBox->addAction(cmd.fillToggle.createAction(
        fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
    QString toolTip = tr("Fill polygon, if closed");
    if (!cmd.fillToggle.getKeySequences().isEmpty()) {
      toolTip += " (" %
          cmd.fillToggle.getKeySequences().first().toString(
              QKeySequence::NativeText) %
          ")";
    }
    fillCheckBox->setToolTip(toolTip);
    connect(fillCheckBox.get(), &QCheckBox::toggled, this,
            &PackageEditorState_DrawPolygonBase::fillCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(fillCheckBox), 10);
  }

  if ((mMode == Mode::RECT) || (mMode == Mode::POLYGON)) {
    std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
    grabAreaCheckBox->setChecked(mLastGrabArea);
    grabAreaCheckBox->addAction(cmd.grabAreaToggle.createAction(
        grabAreaCheckBox.get(), grabAreaCheckBox.get(), &QCheckBox::toggle));
    QString toolTip = tr("Use polygon as grab area");
    if (!cmd.grabAreaToggle.getKeySequences().isEmpty()) {
      toolTip += " (" %
          cmd.grabAreaToggle.getKeySequences().first().toString(
              QKeySequence::NativeText) %
          ")";
    }
    grabAreaCheckBox->setToolTip(toolTip);
    connect(
        grabAreaCheckBox.get(), &QCheckBox::toggled, this,
        &PackageEditorState_DrawPolygonBase::grabAreaCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(grabAreaCheckBox));
  }

  mLastScenePos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  updateCursorPosition(0);
  updateStatusBarMessage();

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
  mContext.graphicsView.setInfoBoxText(QString());
  emit statusBarMessageChanged(QString());
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
    // Reset members.
    if (mMode == Mode::ARC) {
      mLastAngle = Angle(0);
      mArcCenter = mCursorPos;
      mArcInSecondState = false;
    }

    // Create inital path.
    Path path;
    if (mMode == Mode::ARC) {
      for (int i = 0; i < 3; ++i) {
        path.addVertex(mCursorPos);
      }
    } else {
      for (int i = 0; i < ((mMode == Mode::RECT) ? 5 : 2); ++i) {
        path.addVertex(mCursorPos, (i == 0) ? mLastAngle : Angle::deg0());
      }
    }

    // Add polygon.
    mContext.undoStack.beginCmdGroup(tr("Add footprint polygon"));
    mIsUndoCmdActive = true;
    mCurrentPolygon = std::make_shared<Polygon>(Uuid::createRandom(),
                                                *mLastLayer, mLastLineWidth,
                                                mLastFill, mLastGrabArea, path);
    mContext.undoStack.appendToCmdGroup(new CmdPolygonInsert(
        mContext.currentFootprint->getPolygons(), mCurrentPolygon));
    mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentPolygon);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    updateOverlayText();
    updateStatusBarMessage();
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
    updateOverlayText();
    updateStatusBarMessage();
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
    } else if (mMode == Mode::ARC) {
      // Take radius or arc angle into account, depending on state.
      if (!mArcInSecondState) {
        isEmpty = (vertices[vertices.count() - 1].getPos() == mArcCenter);
      } else {
        isEmpty =
            (vertices[vertices.count() - 1].getPos() == vertices[0].getPos());
      }
    } else {
      // Only take the last line segment into account.
      isEmpty = (vertices[vertices.count() - 1].getPos() ==
                 vertices[vertices.count() - 2].getPos());
    }
    if (isEmpty) {
      return abort();
    }

    // If the first part of an arc was drawn, start the second part now.
    if ((mMode == Mode::ARC) && (!mArcInSecondState)) {
      mArcInSecondState = true;
      updatePolygonPath();
      updateOverlayText();
      updateStatusBarMessage();
      return true;
    }

    // Commit current polygon segment.
    mEditCmd->setPath(Path(vertices), true);
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the polygon is completed, abort now.
    if ((mMode == Mode::RECT) || (mMode == Mode::ARC) ||
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
    updateOverlayText();
    updateStatusBarMessage();
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

  updateOverlayText();
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
  } else if (mMode == Mode::ARC) {
    if (!mArcInSecondState) {
      // Draw 2 arcs with 180° each to result in an accurate 360° circle.
      // This circle helps the user to place the start point of the arc.
      Q_ASSERT(count == 3);
      vertices[2] = Vertex(mCursorPos);
      vertices[1] = Vertex(mCursorPos.rotated(Angle::deg180(), mArcCenter),
                           Angle::deg180());
      vertices[0] = Vertex(mCursorPos, Angle::deg180());
    } else {
      // Now place the end point of the arc. The only degree of freedom is the
      // angle. This angle is determined by the current cursor position and
      // the position where the cursor was before to determine the arc's
      // direction.
      Point arcStart = vertices[0].getPos();
      Angle angle =
          Toolbox::arcAngle(arcStart, mCursorPos, mArcCenter).mappedTo180deg();
      if (((mLastAngle > Angle::deg90()) && (angle < 0)) ||
          ((mLastAngle < -Angle::deg90()) && (angle > 0))) {
        angle.invert();
      }
      // Remove the old arc segments.
      while (vertices.count() > 1) {
        vertices.removeLast();
      }
      if (angle.abs() > Angle::deg270()) {
        // The angle is > 270°, so let's create two separate arc segments to
        // avoid mathematical inaccuracy due to too high angle.
        Angle halfAngle = angle / 2;
        vertices[0].setAngle(angle - halfAngle);
        vertices.append(
            Vertex(arcStart.rotated(halfAngle, mArcCenter), halfAngle));
        vertices.append(Vertex(arcStart.rotated(angle, mArcCenter)));
      } else {
        // The angle is small enough to be implemented by a single arc segment.
        vertices[0].setAngle(angle);
        vertices.append(Vertex(arcStart.rotated(angle, mArcCenter)));
      }
      mLastAngle = angle;
    }
  } else {
    Q_ASSERT(count >= 2);
    vertices[count - 1].setPos(mCursorPos);
  }
  mEditCmd->setPath(Path(vertices), true);
}

void PackageEditorState_DrawPolygonBase::updateOverlayText() noexcept {
  const LengthUnit& unit = getLengthUnit();
  const int decimals = unit.getReasonableNumberOfDecimals();
  auto formatLength = [&unit, decimals](const QString& name,
                                        const Length& value) {
    return QString("%1: %2 %3")
        .arg(name)
        .arg(unit.convertToUnit(value), 11 - name.length(), 'f', decimals)
        .arg(unit.toShortStringTr());
  };
  auto formatAngle = [decimals](const QString& name, const Angle& value) {
    return QString("%1: %2°").arg(name).arg(
        value.toDeg(), 14 - decimals - name.length(), 'f', 3);
  };

  const QVector<Vertex> vertices = mCurrentPolygon
      ? mCurrentPolygon->getPath().getVertices()
      : QVector<Vertex>{};
  const int count = vertices.count();

  QString text;
  switch (mMode) {
    case Mode::LINE:
    case Mode::POLYGON: {
      const Point p0 = (count >= 2) ? vertices[count - 2].getPos() : mCursorPos;
      const Point p1 = (count >= 2) ? vertices[count - 1].getPos() : mCursorPos;
      const Point diff = p1 - p0;
      const UnsignedLength length = (p1 - p0).getLength();
      const Angle angle = Angle::fromRad(
          qAtan2(diff.toMmQPointF().y(), diff.toMmQPointF().x()));
      text += formatLength("X0", p0.getX()) % "<br>";
      text += formatLength("Y0", p0.getY()) % "<br>";
      text += formatLength("X1", p1.getX()) % "<br>";
      text += formatLength("Y0", p1.getY()) % "<br>";
      text += "<br>";
      text += "<b>" % formatLength("Δ", *length) % "</b><br>";
      text += "<b>" % formatAngle("∠", angle) % "</b>";
      break;
    }
    case Mode::RECT: {
      const Point p0 = (count >= 3) ? vertices[0].getPos() : mCursorPos;
      const Point p1 = (count >= 3) ? vertices[2].getPos() : mCursorPos;
      const Length width = (p1.getX() - p0.getX()).abs();
      const Length height = (p1.getY() - p0.getY()).abs();
      text += formatLength("X0", p0.getX()) % "<br>";
      text += formatLength("Y0", p0.getY()) % "<br>";
      text += formatLength("X1", p1.getX()) % "<br>";
      text += formatLength("Y0", p1.getY()) % "<br>";
      text += "<br>";
      text += "<b>" % formatLength("ΔX", width) % "</b><br>";
      text += "<b>" % formatLength("ΔY", height) % "</b>";
      break;
    }
    case Mode::ARC: {
      const Point center = (count >= 2) ? mArcCenter : mCursorPos;
      const Point p0 = (count >= 2) ? vertices.first().getPos() : mCursorPos;
      const Point p1 = (count >= 2) ? vertices.last().getPos() : mCursorPos;
      const UnsignedLength radius =
          (count >= 2) ? (p0 - mArcCenter).getLength() : UnsignedLength(0);
      Angle angle;
      foreach (const Vertex& v, vertices) {
        angle += v.getAngle();
      }
      text += formatLength("X·", center.getX()) % "<br>";
      text += formatLength("Y·", center.getY()) % "<br>";
      text += formatLength("X0", p0.getX()) % "<br>";
      text += formatLength("Y0", p0.getY()) % "<br>";
      text += formatLength("X1", p1.getX()) % "<br>";
      text += formatLength("Y1", p1.getY()) % "<br>";
      text += "<br>";
      text += "<b>" % formatLength("r", *radius) % "</b><br>";
      text += "<b>" % formatLength("⌀", radius * 2) % "</b><br>";
      text += "<b>" % formatAngle("∠", angle) % "</b>";
      break;
    }
    default: {
      qWarning() << "Unhandled switch-case in "
                    "PackageEditorState_DrawPolygonBase::updateOverlayText():"
                 << static_cast<int>(mMode);
      break;
    }
  }

  text.replace(" ", "&nbsp;");
  mContext.graphicsView.setInfoBoxText(text);
}

void PackageEditorState_DrawPolygonBase::updateStatusBarMessage() noexcept {
  QString note = " " %
      tr("(press %1 to disable snap, %2 to abort)")
          .arg(QCoreApplication::translate("QShortcut", "Shift"))
          .arg(tr("right click"));

  if (mMode == Mode::RECT) {
    if (!mIsUndoCmdActive) {
      emit statusBarMessageChanged(tr("Click to specify the first edge") %
                                   note);
    } else {
      emit statusBarMessageChanged(tr("Click to specify the second edge") %
                                   note);
    }
  } else if (mMode == Mode::ARC) {
    if (!mIsUndoCmdActive) {
      emit statusBarMessageChanged(tr("Click to specify the arc center") %
                                   note);
    } else if (!mArcInSecondState) {
      emit statusBarMessageChanged(tr("Click to specify the start point") %
                                   note);
    } else {
      emit statusBarMessageChanged(tr("Click to specify the end point") % note);
    }
  } else {
    if (!mIsUndoCmdActive) {
      emit statusBarMessageChanged(tr("Click to specify the first point") %
                                   note);
    } else {
      emit statusBarMessageChanged(tr("Click to specify the next point") %
                                   note);
    }
  }
}

void PackageEditorState_DrawPolygonBase::layerComboBoxValueChanged(
    const Layer& layer) noexcept {
  mLastLayer = &layer;
  if (mEditCmd) {
    mEditCmd->setLayer(*mLastLayer, true);
  }
  if (mUsedLineWidths.contains(&layer)) {
    emit requestLineWidth(*mUsedLineWidths.find(&layer));
  } else if (layer.getPolygonsRepresentAreas()) {
    // Zero-width polygons.
    emit requestLineWidth(UnsignedLength(0));
  } else {
    // Typical width according library conventions.
    emit requestLineWidth(UnsignedLength(200000));
  }
}

void PackageEditorState_DrawPolygonBase::lineWidthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastLineWidth = value;
  if (mEditCmd) {
    mEditCmd->setLineWidth(mLastLineWidth, true);
  }
  mUsedLineWidths.insert(mLastLayer, value);
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
