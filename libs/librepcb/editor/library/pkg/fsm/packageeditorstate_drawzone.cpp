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
#include "packageeditorstate_drawzone.h"

#include "../../../cmd/cmdzoneedit.h"
#include "../../../graphics/zonegraphicsitem.h"
#include "../../../undostack.h"
#include "../footprintgraphicsitem.h"

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

PackageEditorState_DrawZone::PackageEditorState_DrawZone(
    Context& context) noexcept
  : PackageEditorState(context),
    mLastAngle(0),
    mIsUndoCmdActive(false),
    mCurrentProperties(Uuid::createRandom(),  // Not relevant
                       Zone::Layer::Top,  // Layers
                       Zone::Rule::All,  // Rules,
                       Path()  // Not relevant
                       ),
    mCurrentZone(nullptr),
    mCurrentGraphicsItem(nullptr) {
}

PackageEditorState_DrawZone::~PackageEditorState_DrawZone() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawZone::entry() noexcept {
  mLastScenePos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                      .mappedToGrid(getGridInterval());
  updateCursorPosition(Qt::KeyboardModifier::NoModifier);
  updateStatusBarMessage();

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawZone::exit() noexcept {
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

bool PackageEditorState_DrawZone::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (e.key == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers);
    return true;
  }

  return false;
}

bool PackageEditorState_DrawZone::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (e.key == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers);
    return true;
  }

  return false;
}

bool PackageEditorState_DrawZone::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = e.scenePos;
  updateCursorPosition(e.modifiers);
  return true;
}

bool PackageEditorState_DrawZone::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = e.scenePos;
  if (mIsUndoCmdActive) {
    return addNextSegment();
  } else {
    return start();
  }
}

bool PackageEditorState_DrawZone::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  // Handle like a single click.
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool PackageEditorState_DrawZone::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    return abort();
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void PackageEditorState_DrawZone::setLayer(Zone::Layer layer,
                                           bool enable) noexcept {
  Zone::Layers layers = mCurrentProperties.getLayers();
  layers.setFlag(layer, enable);

  if (mCurrentProperties.setLayers(layers)) {
    emit layersChanged(mCurrentProperties.getLayers());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLayers(mCurrentProperties.getLayers(), true);
  }
}

void PackageEditorState_DrawZone::setRule(Zone::Rule rule,
                                          bool enable) noexcept {
  Zone::Rules rules = mCurrentProperties.getRules();
  rules.setFlag(rule, enable);

  if (mCurrentProperties.setRules(rules)) {
    emit rulesChanged(mCurrentProperties.getRules());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setRules(mCurrentProperties.getRules(), true);
  }
}

void PackageEditorState_DrawZone::setAngle(const Angle& angle) noexcept {
  if (angle != mLastAngle) {
    mLastAngle = angle;
    emit angleChanged(mLastAngle);
  }

  if (mCurrentZone && mCurrentEditCmd) {
    Path path = mCurrentZone->getOutline();
    Q_ASSERT(path.getVertices().count() >= 2);
    path.getVertices()[path.getVertices().count() - 2].setAngle(mLastAngle);
    mCurrentEditCmd->setOutline(path, true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawZone::start() noexcept {
  if (!mContext.currentGraphicsItem) return false;

  try {
    // Create inital path.
    mCurrentProperties.setOutline(Path({
        Vertex(mCursorPos, mLastAngle),
        Vertex(mCursorPos),
    }));

    // Add zone.
    mContext.undoStack.beginCmdGroup(tr("Add Footprint Zone"));
    mIsUndoCmdActive = true;
    mCurrentZone =
        std::make_shared<Zone>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(
        new CmdZoneInsert(mContext.currentFootprint->getZones(), mCurrentZone));
    mCurrentEditCmd.reset(new CmdZoneEdit(*mCurrentZone));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentZone);
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

bool PackageEditorState_DrawZone::abort(bool showErrMsgBox) noexcept {
  try {
    if (mCurrentGraphicsItem) {
      mCurrentGraphicsItem->setSelected(false);
      mCurrentGraphicsItem.reset();
    }
    mCurrentEditCmd.reset();
    mCurrentZone.reset();
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

bool PackageEditorState_DrawZone::addNextSegment() noexcept {
  try {
    // If no line was drawn, abort now.
    QVector<Vertex> vertices = mCurrentZone->getOutline().getVertices();
    const bool isEmpty = (vertices[vertices.count() - 1].getPos() ==
                          vertices[vertices.count() - 2].getPos());
    if (isEmpty) {
      return abort();
    }

    // If the outline is closed, remove the last vertex.
    const bool closed = (vertices.first().getPos() == vertices.last().getPos());
    if (closed) {
      vertices.removeLast();
    }

    // Commit current segment.
    mCurrentEditCmd->setOutline(Path(vertices), true);
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the outline is closed, abort now.
    if (closed) {
      return abort();
    }

    // Add next segment.
    mContext.undoStack.beginCmdGroup(tr("Add Footprint Zone"));
    mIsUndoCmdActive = true;
    mCurrentEditCmd.reset(new CmdZoneEdit(*mCurrentZone));
    vertices.last().setAngle(mLastAngle);
    vertices.append(Vertex(mCursorPos, Angle::deg0()));
    mCurrentEditCmd->setOutline(Path(vertices), true);
    updateOverlayText();
    updateStatusBarMessage();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_DrawZone::updateCursorPosition(
    Qt::KeyboardModifiers modifiers) noexcept {
  mCursorPos = mLastScenePos;
  if (!modifiers.testFlag(Qt::ShiftModifier)) {
    mCursorPos.mapToGrid(getGridInterval());
  }
  mAdapter.fsmSetSceneCursor(mCursorPos, true, false);

  if (mCurrentZone && mCurrentEditCmd) {
    updateOutline();
  }

  updateOverlayText();
}

void PackageEditorState_DrawZone::updateOutline() noexcept {
  QVector<Vertex> vertices = mCurrentZone->getOutline().getVertices();
  Q_ASSERT(vertices.count() >= 2);
  vertices.last().setPos(mCursorPos);
  mCurrentEditCmd->setOutline(Path(vertices), true);
}

void PackageEditorState_DrawZone::updateOverlayText() noexcept {
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

  const QVector<Vertex> vertices = mCurrentZone
      ? mCurrentZone->getOutline().getVertices()
      : QVector<Vertex>{};
  const int count = vertices.count();

  QString text;
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
  text.replace(" ", "&nbsp;");
  mAdapter.fsmSetViewInfoBoxText(text);
}

void PackageEditorState_DrawZone::updateStatusBarMessage() noexcept {
  QString note = " " %
      tr("(press %1 to disable snap, %2 to abort)")
          .arg(QCoreApplication::translate("QShortcut", "Shift"))
          .arg(tr("right click"));

  if (!mIsUndoCmdActive) {
    mAdapter.fsmSetStatusBarMessage(tr("Click to specify the first point") %
                                    note);
  } else {
    mAdapter.fsmSetStatusBarMessage(tr("Click to specify the next point") %
                                    note);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
