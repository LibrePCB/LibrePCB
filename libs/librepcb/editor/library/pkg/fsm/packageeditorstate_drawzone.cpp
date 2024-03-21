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
#include "../../../widgets/angleedit.h"
#include "../../../widgets/graphicsview.h"
#include "../footprintgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/geometry/zone.h>
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
    mIsUndoCmdActive(false),
    mCurrentZone(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayers(Zone::Layer::Top),
    mLastRules(Zone::Rule::All),
    mLastAngle(0) {
}

PackageEditorState_DrawZone::~PackageEditorState_DrawZone() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_DrawZone::entry() noexcept {
  // populate command toolbar
  std::unique_ptr<QCheckBox> cbxTop(new QCheckBox(tr("Top")));
  cbxTop->setChecked(mLastLayers.testFlag(Zone::Layer::Top));
  connect(cbxTop.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastLayers.setFlag(Zone::Layer::Top, checked);
    if (mEditCmd) {
      mEditCmd->setLayers(mLastLayers, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxTop));

  std::unique_ptr<QCheckBox> cbxInner(new QCheckBox(tr("Inner")));
  cbxInner->setChecked(mLastLayers.testFlag(Zone::Layer::Inner));
  connect(cbxInner.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastLayers.setFlag(Zone::Layer::Inner, checked);
    if (mEditCmd) {
      mEditCmd->setLayers(mLastLayers, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxInner));

  std::unique_ptr<QCheckBox> cbxBottom(new QCheckBox(tr("Bottom")));
  cbxBottom->setChecked(mLastLayers.testFlag(Zone::Layer::Bottom));
  connect(cbxBottom.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastLayers.setFlag(Zone::Layer::Bottom, checked);
    if (mEditCmd) {
      mEditCmd->setLayers(mLastLayers, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxBottom));
  mContext.commandToolBar.addSeparator();

  std::unique_ptr<QCheckBox> cbxNoCopper(new QCheckBox(tr("No Copper")));
  cbxNoCopper->setChecked(mLastRules.testFlag(Zone::Rule::NoCopper));
  connect(cbxNoCopper.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoCopper, checked);
    if (mEditCmd) {
      mEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoCopper));

  std::unique_ptr<QCheckBox> cbxNoPlanes(new QCheckBox(tr("No Planes")));
  cbxNoPlanes->setChecked(mLastRules.testFlag(Zone::Rule::NoPlanes));
  connect(cbxNoPlanes.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoPlanes, checked);
    if (mEditCmd) {
      mEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoPlanes));

  std::unique_ptr<QCheckBox> cbxNoExposure(new QCheckBox(tr("No Exposure")));
  cbxNoExposure->setChecked(mLastRules.testFlag(Zone::Rule::NoExposure));
  connect(cbxNoExposure.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoExposure, checked);
    if (mEditCmd) {
      mEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoExposure));

  std::unique_ptr<QCheckBox> cbxNoDevices(new QCheckBox(tr("No Devices")));
  cbxNoDevices->setChecked(mLastRules.testFlag(Zone::Rule::NoDevices));
  connect(cbxNoDevices.get(), &QCheckBox::toggled, this, [this](bool checked) {
    mLastRules.setFlag(Zone::Rule::NoDevices, checked);
    if (mEditCmd) {
      mEditCmd->setRules(mLastRules, true);
    }
  });
  mContext.commandToolBar.addWidget(std::move(cbxNoDevices));
  mContext.commandToolBar.addSeparator();

  mContext.commandToolBar.addLabel(tr("Arc Angle:"), 10);
  std::unique_ptr<AngleEdit> edtAngle(new AngleEdit());
  edtAngle->setSingleStep(90.0);  // [°]
  edtAngle->setValue(mLastAngle);
  connect(edtAngle.get(), &AngleEdit::valueChanged, this,
          &PackageEditorState_DrawZone::angleEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtAngle));

  mLastScenePos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  updateCursorPosition(Qt::KeyboardModifier::NoModifier);
  updateStatusBarMessage();

  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_DrawZone::exit() noexcept {
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
    PackageEditorState_DrawZone::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_DrawZone::processKeyPressed(
    const QKeyEvent& e) noexcept {
  if (e.key() == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers());
    return true;
  }

  return false;
}

bool PackageEditorState_DrawZone::processKeyReleased(
    const QKeyEvent& e) noexcept {
  if (e.key() == Qt::Key_Shift) {
    updateCursorPosition(e.modifiers());
    return true;
  }

  return false;
}

bool PackageEditorState_DrawZone::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = Point::fromPx(e.scenePos());
  updateCursorPosition(e.modifiers());
  return true;
}

bool PackageEditorState_DrawZone::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  mLastScenePos = Point::fromPx(e.scenePos());
  if (mIsUndoCmdActive) {
    return addNextSegment();
  } else {
    return start();
  }
}

bool PackageEditorState_DrawZone::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
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
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_DrawZone::start() noexcept {
  try {
    // Create inital path.
    Path path({
        Vertex(mCursorPos, mLastAngle),
        Vertex(mCursorPos),
    });

    // Add zone.
    mContext.undoStack.beginCmdGroup(tr("Add footprint zone"));
    mIsUndoCmdActive = true;
    mCurrentZone = std::make_shared<Zone>(Uuid::createRandom(), mLastLayers,
                                          mLastRules, path);
    mContext.undoStack.appendToCmdGroup(
        new CmdZoneInsert(mContext.currentFootprint->getZones(), mCurrentZone));
    mEditCmd.reset(new CmdZoneEdit(*mCurrentZone));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentZone);
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

bool PackageEditorState_DrawZone::abort(bool showErrMsgBox) noexcept {
  try {
    if (mCurrentGraphicsItem) {
      mCurrentGraphicsItem->setSelected(false);
      mCurrentGraphicsItem.reset();
    }
    mEditCmd.reset();
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
      QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
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
    mEditCmd->setOutline(Path(vertices), true);
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the outline is closed, abort now.
    if (closed) {
      return abort();
    }

    // Add next segment.
    mContext.undoStack.beginCmdGroup(tr("Add footprint zone"));
    mIsUndoCmdActive = true;
    mEditCmd.reset(new CmdZoneEdit(*mCurrentZone));
    vertices.last().setAngle(mLastAngle);
    vertices.append(Vertex(mCursorPos, Angle::deg0()));
    mEditCmd->setOutline(Path(vertices), true);
    updateOverlayText();
    updateStatusBarMessage();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_DrawZone::updateCursorPosition(
    Qt::KeyboardModifiers modifiers) noexcept {
  mCursorPos = mLastScenePos;
  if (!modifiers.testFlag(Qt::ShiftModifier)) {
    mCursorPos.mapToGrid(getGridInterval());
  }
  mContext.graphicsView.setSceneCursor(
      std::make_pair(mCursorPos, GraphicsView::CursorOption::Cross));

  if (mCurrentZone && mEditCmd) {
    updateOutline();
  }

  updateOverlayText();
}

void PackageEditorState_DrawZone::updateOutline() noexcept {
  QVector<Vertex> vertices = mCurrentZone->getOutline().getVertices();
  Q_ASSERT(vertices.count() >= 2);
  vertices.last().setPos(mCursorPos);
  mEditCmd->setOutline(Path(vertices), true);
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
  mContext.graphicsView.setInfoBoxText(text);
}

void PackageEditorState_DrawZone::updateStatusBarMessage() noexcept {
  QString note = " " %
      tr("(press %1 to disable snap, %2 to abort)")
          .arg(QCoreApplication::translate("QShortcut", "Shift"))
          .arg(tr("right click"));

  if (!mIsUndoCmdActive) {
    emit statusBarMessageChanged(tr("Click to specify the first point") % note);
  } else {
    emit statusBarMessageChanged(tr("Click to specify the next point") % note);
  }
}

void PackageEditorState_DrawZone::angleEditValueChanged(
    const Angle& value) noexcept {
  mLastAngle = value;
  if (mCurrentZone && mEditCmd) {
    Path path = mCurrentZone->getOutline();
    Q_ASSERT(path.getVertices().count() >= 2);
    path.getVertices()[path.getVertices().count() - 2].setAngle(mLastAngle);
    mEditCmd->setOutline(path, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
