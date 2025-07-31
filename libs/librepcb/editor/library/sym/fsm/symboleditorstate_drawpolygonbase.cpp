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
#include "symboleditorstate_drawpolygonbase.h"

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../undostack.h"
#include "../symbolgraphicsitem.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/library/sym/symbol.h>
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

SymbolEditorState_DrawPolygonBase::SymbolEditorState_DrawPolygonBase(
    const Context& context, Mode mode) noexcept
  : SymbolEditorState(context),
    mMode(mode),
    mIsUndoCmdActive(false),
    mArcInSecondState(false),
    mCurrentProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        Layer::symbolOutlines(),  // Most important layer
        UnsignedLength(200000),  // Typical width according library conventions
        false,  // Is filled
        (mode != Mode::LINE) && (mode != Mode::ARC),  // Is grab area
        Path()  // Path is not relevant here
        ),
    mCurrentPolygon(nullptr),
    mCurrentEditCmd(nullptr) {
}

SymbolEditorState_DrawPolygonBase::
    ~SymbolEditorState_DrawPolygonBase() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_DrawPolygonBase::entry() noexcept {
  mLastScenePos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                      .mappedToGrid(getGridInterval());
  updateCursorPosition(Qt::KeyboardModifier::NoModifier);
  updateStatusBarMessage();

  notifyToolEnter();
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_DrawPolygonBase::exit() noexcept {
  if (!abort()) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetSceneCursor(Point(), false, false);
  mAdapter.fsmSetViewInfoBoxText(QString());
  mAdapter.fsmSetStatusBarMessage(QString());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_DrawPolygonBase::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (e.key == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers);
    return true;
  }

  return false;
}

bool SymbolEditorState_DrawPolygonBase::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (e.key == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers);
    return true;
  }

  return false;
}

bool SymbolEditorState_DrawPolygonBase::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = e.scenePos;
  updateCursorPosition(e.modifiers);
  return true;
}

bool SymbolEditorState_DrawPolygonBase::
    processGraphicsSceneLeftMouseButtonPressed(
        const GraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = e.scenePos;
  if (mIsUndoCmdActive) {
    return addNextSegment();
  } else {
    return start();
  }
}

bool SymbolEditorState_DrawPolygonBase::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  // Handle like a single click.
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SymbolEditorState_DrawPolygonBase::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    return abort();
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> SymbolEditorState_DrawPolygonBase::getAvailableLayers()
    const noexcept {
  return getAllowedCircleAndPolygonLayers();
}

void SymbolEditorState_DrawPolygonBase::setLayer(const Layer& layer) noexcept {
  if (mCurrentProperties.setLayer(layer)) {
    emit layerChanged(mCurrentProperties.getLayer());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLayer(mCurrentProperties.getLayer(), true);
  }
}

void SymbolEditorState_DrawPolygonBase::setLineWidth(
    const UnsignedLength& width) noexcept {
  if (mCurrentProperties.setLineWidth(width)) {
    emit lineWidthChanged(mCurrentProperties.getLineWidth());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLineWidth(mCurrentProperties.getLineWidth(), true);
  }
}

void SymbolEditorState_DrawPolygonBase::setFilled(bool filled) noexcept {
  if (mCurrentProperties.setIsFilled(filled)) {
    emit filledChanged(mCurrentProperties.isFilled());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setIsFilled(mCurrentProperties.isFilled(), true);
  }
}

void SymbolEditorState_DrawPolygonBase::setGrabArea(bool grabArea) noexcept {
  if (mCurrentProperties.setIsGrabArea(grabArea)) {
    emit grabAreaChanged(mCurrentProperties.isGrabArea());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setIsGrabArea(mCurrentProperties.isGrabArea(), true);
  }
}

void SymbolEditorState_DrawPolygonBase::setAngle(const Angle& angle) noexcept {
  if (angle != mLastAngle) {
    mLastAngle = angle;
    emit angleChanged(mLastAngle);
  }

  if (mCurrentPolygon && mCurrentEditCmd) {
    Path path = mCurrentPolygon->getPath();
    if (path.getVertices().count() > 1) {
      path.getVertices()[path.getVertices().count() - 2].setAngle(mLastAngle);
      mCurrentEditCmd->setPath(path, true);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_DrawPolygonBase::start() noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

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
    mCurrentProperties.setPath(path);

    // Add polygon.
    mContext.undoStack.beginCmdGroup(tr("Add symbol polygon"));
    mIsUndoCmdActive = true;
    mCurrentPolygon =
        std::make_shared<Polygon>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(
        new CmdPolygonInsert(mContext.symbol.getPolygons(), mCurrentPolygon));
    mCurrentEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    mCurrentGraphicsItem = item->getGraphicsItem(mCurrentPolygon);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    updateOverlayText();
    updateStatusBarMessage();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abort(false);
    return false;
  }
}

bool SymbolEditorState_DrawPolygonBase::abort(bool showErrMsgBox) noexcept {
  try {
    if (mCurrentGraphicsItem) {
      mCurrentGraphicsItem->setSelected(false);
      mCurrentGraphicsItem.reset();
    }
    mCurrentEditCmd.reset();
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
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

bool SymbolEditorState_DrawPolygonBase::addNextSegment() noexcept {
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
    mCurrentEditCmd->setPath(Path(vertices), true);
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the polygon is completed, abort now.
    if ((mMode == Mode::RECT) || (mMode == Mode::ARC) ||
        (vertices.first().getPos() == vertices.last().getPos())) {
      return abort();
    }

    // Add next polygon segment.
    mContext.undoStack.beginCmdGroup(tr("Add symbol polygon"));
    mIsUndoCmdActive = true;
    mCurrentEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
    vertices.last().setAngle(mLastAngle);
    vertices.append(Vertex(mCursorPos, Angle::deg0()));
    mCurrentEditCmd->setPath(Path(vertices), true);
    updateOverlayText();
    updateStatusBarMessage();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void SymbolEditorState_DrawPolygonBase::updateCursorPosition(
    Qt::KeyboardModifiers modifiers) noexcept {
  mCursorPos = mLastScenePos;
  if (!modifiers.testFlag(Qt::ShiftModifier)) {
    mCursorPos.mapToGrid(getGridInterval());
  }
  mAdapter.fsmSetSceneCursor(mCursorPos, true, false);

  if (mCurrentPolygon && mCurrentEditCmd) {
    updatePolygonPath();
  }

  updateOverlayText();
}

void SymbolEditorState_DrawPolygonBase::updatePolygonPath() noexcept {
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
  mCurrentEditCmd->setPath(Path(vertices), true);
}

void SymbolEditorState_DrawPolygonBase::updateOverlayText() noexcept {
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
          std::atan2(diff.toMmQPointF().y(), diff.toMmQPointF().x()));
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
                    "SymbolEditorState_DrawPolygonBase::updateOverlayText():"
                 << static_cast<int>(mMode);
      break;
    }
  }

  text.replace(" ", "&nbsp;");
  mAdapter.fsmSetViewInfoBoxText(text);
}

void SymbolEditorState_DrawPolygonBase::updateStatusBarMessage() noexcept {
  QString note = " " %
      tr("(press %1 to disable snap, %2 to abort)")
          .arg(QCoreApplication::translate("QShortcut", "Shift"))
          .arg(tr("right click"));

  if (mMode == Mode::RECT) {
    if (!mIsUndoCmdActive) {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the first edge") %
                                      note);
    } else {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the second edge") %
                                      note);
    }
  } else if (mMode == Mode::ARC) {
    if (!mIsUndoCmdActive) {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the arc center") %
                                      note);
    } else if (!mArcInSecondState) {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the start point") %
                                      note);
    } else {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the end point") %
                                      note);
    }
  } else {
    if (!mIsUndoCmdActive) {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the first point") %
                                      note);
    } else {
      mAdapter.fsmSetStatusBarMessage(tr("Click to specify the next point") %
                                      note);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
