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
#include "board3dtab.h"

#include "../projectsmodel.h"
#include "apptoolbox.h"
#include "guiapplication.h"
#include "project/projecteditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/editor/3d/openglscenebuilder.h>
#include <librepcb/editor/undostack.h>
#include <librepcb/editor/widgets/openglview.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Board3dTab::Board3dTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                       int boardIndex, QObject* parent) noexcept
  : WindowTab(app, prj, boardIndex, parent),
    mEditor(prj),
    mUiData{q2s(QColor(Qt::white)), q2s(QColor(Qt::black))},
    mAnimation(new QVariantAnimation(this)) {
  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            applyProjection(mAnimationDataStart.interpolated(
                mAnimationDataDelta, value.toReal()));
          });

  // Connect undo stack.
  connect(&mEditor->getUndoStack(), &UndoStack::stateModified, this,
          &Board3dTab::uiDataChanged);
  connect(mEditor.get(), &ProjectEditor::manualModificationsMade, this,
          &Board3dTab::uiDataChanged);
}

Board3dTab::~Board3dTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData Board3dTab::getBaseUiData() const noexcept {
  auto brd = mProject->getProject().getBoardByIndex(mObjIndex);

  return ui::TabData{
      ui::TabType::Board3d,  // Type
      q2s(brd ? *brd->getName() : QString()),  // Title
      q2s(QPixmap(":/3d.svg")),  // Icon
      mApp.getProjects().getIndexOf(mEditor),  // Project index
      ui::RuleCheckState::NotAvailable,  // Rule check state
      nullptr,  // Rule check messages
      slint::SharedString(),  // Rule check execution error
      mEditor->canSave(),  // Can save
      false,  // Can export graphics
      mProject->getUndoStack().canUndo(),  // Can undo
      q2s(mProject->getUndoStack().getUndoCmdText()),  // Undo text
      mProject->getUndoStack().canRedo(),  // Can redo
      q2s(mProject->getUndoStack().getRedoCmdText()),  // Redo text
      false,  // Can cut/copy
      false,  // Can paste
      false,  // Can remove
      false,  // Can rotate
      false,  // Can mirror
  };
}

void Board3dTab::setUiData(const ui::Board3dTabData& data) noexcept {
  mUiData = data;
}

void Board3dTab::activate() noexcept {
  if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
    mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
    connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
            [this](BoardPlaneFragmentsBuilder::Result result) {
              if (result.applyToBoard()) {
                emit requestRepaint();
              }
            });
    mPlaneBuilder->start(*brd);
    mOpenGlView.reset(new OpenGlView());
    mOpenGlView->setTransform(mProjection.transform, mProjection.fov,
                              mProjection.center);
    mOpenGlSceneBuilder.reset(new OpenGlSceneBuilder(this));
    connect(mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded,
            mOpenGlView.get(), &OpenGlView::addObject);
    connect(
        mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded, this,
        [this]() { emit requestRepaint(); }, Qt::QueuedConnection);
    auto av =
        mProject->getProject().getCircuit().getAssemblyVariants().value(0);
    mOpenGlSceneBuilder->start(brd->buildScene3D(
        av ? std::make_optional(av->getUuid()) : std::nullopt));
    mUiData.overlay_color = q2s(QColor(Qt::black));
    emit requestRepaint();
  }
}

void Board3dTab::deactivate() noexcept {
  mPlaneBuilder.reset();
  mOpenGlView.reset();
  mOpenGlSceneBuilder.reset();
}

bool Board3dTab::actionTriggered(ui::ActionId id) noexcept {
  if (id == ui::ActionId::Save) {
    mEditor->saveProject();
    return true;
  }

  return WindowTab::actionTriggered(id);
}

slint::Image Board3dTab::renderScene(float width, float height) noexcept {
  if (mOpenGlView) {
    mOpenGlView->resize(width, height);
    return q2s(mOpenGlView->grab());
  }
  return slint::Image();
}

bool Board3dTab::processScenePointerEvent(
    const QPointF& pos, const QPointF& globalPos,
    slint::private_api::PointerEvent e) noexcept {
  Q_UNUSED(globalPos);
  Projection projection = mProjection;
  if (mOpenGlView) {
    if (e.kind == slint::private_api::PointerEventKind::Down) {
      mMousePressPosition = pos;
      mMousePressTransform = projection.transform;
      mMousePressCenter = projection.center;
      buttons.insert(e.button);
    } else if (e.kind == slint::private_api::PointerEventKind::Up) {
      buttons.remove(e.button);
    } else if (e.kind == slint::private_api::PointerEventKind::Move) {
      const QPointF posNorm = mOpenGlView->toNormalizedPos(pos);
      const QPointF mMousePressPosNorm =
          mOpenGlView->toNormalizedPos(mMousePressPosition);

      if (buttons.contains(slint::private_api::PointerEventButton::Middle) ||
          buttons.contains(slint::private_api::PointerEventButton::Right)) {
        const QPointF cursorPosOld =
            mOpenGlView->toModelPos(mMousePressPosNorm);
        const QPointF cursorPosNew = mOpenGlView->toModelPos(posNorm);
        projection.center = mMousePressCenter + cursorPosNew - cursorPosOld;
      }
      if (buttons.contains(slint::private_api::PointerEventButton::Left)) {
        projection.transform = mMousePressTransform;
        if (e.modifiers.shift) {
          // Rotate around Z axis.
          const QPointF p1 =
              mOpenGlView->toModelPos(mMousePressPosNorm) - projection.center;
          const QPointF p2 =
              mOpenGlView->toModelPos(posNorm) - projection.center;
          const qreal angle1 = std::atan2(p1.y(), p1.x());
          const qreal angle2 = std::atan2(p2.y(), p2.x());
          const Angle angle = Angle::fromRad(angle2 - angle1).mappedTo180deg();
          const QVector3D axis = mMousePressTransform.inverted().map(
              QVector3D(0, 0, angle.toDeg()));
          projection.transform.rotate(QQuaternion::fromAxisAndAngle(
              axis.normalized(), angle.abs().toDeg()));
        } else {
          // Rotate around X/Y axis.
          const QVector2D delta(posNorm - mMousePressPosNorm);
          const QVector3D axis = mMousePressTransform.inverted().map(
              QVector3D(-delta.y(), delta.x(), 0));
          projection.transform.rotate(QQuaternion::fromAxisAndAngle(
              axis.normalized(), delta.length() * 270));
        }
      }
    }
  }
  return applyProjection(projection);
}

bool Board3dTab::processSceneScrolled(
    float x, float y, slint::private_api::PointerScrollEvent e) noexcept {
  const qreal factor = qPow(1.3, e.delta_y / qreal(120));
  return zoom(QPointF(x, y), factor);
}

void Board3dTab::zoomFit(float width, float height) noexcept {
  Q_UNUSED(width);
  Q_UNUSED(height);
  Projection projection = mProjection;
  if (mOpenGlView) {
    projection.fov = sInitialFov;
    projection.center = QPointF();
    projection.transform = QMatrix4x4();
  }
  smoothTo(projection);
}

void Board3dTab::zoomIn(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), 1.3);
}

void Board3dTab::zoomOut(float width, float height) noexcept {
  zoom(QPointF(width / 2, height / 2), 1 / 1.3);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool Board3dTab::zoom(const QPointF& center, qreal factor) noexcept {
  Projection projection = mProjection;
  if (mOpenGlView) {
    const QPointF centerNormalized = mOpenGlView->toNormalizedPos(center);
    const QPointF modelPosOld = mOpenGlView->toModelPos(centerNormalized);

    projection.fov = qBound(qreal(0.01), projection.fov / factor, qreal(90));
    mOpenGlView->setTransform(projection.transform, projection.fov,
                              projection.center);
    const QPointF modelPosNew = mOpenGlView->toModelPos(centerNormalized);
    projection.center += modelPosNew - modelPosOld;
  }
  return applyProjection(projection);
}

void Board3dTab::smoothTo(const Projection& projection) noexcept {
  mAnimationDataStart = mProjection;
  mAnimationDataDelta = projection - mProjection;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

bool Board3dTab::applyProjection(const Projection& projection) noexcept {
  if (projection != mProjection) {
    mProjection = projection;
    if (mOpenGlView) {
      mOpenGlView->setTransform(projection.transform, projection.fov,
                                projection.center);
    }
    // mUiData.frame++;
    // emit uiDataChanged(mUiData.index);
    emit requestRepaint();
    return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
