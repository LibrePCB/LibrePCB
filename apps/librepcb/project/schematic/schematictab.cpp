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
#include "schematictab.h"

#include "../projecteditor.h"
#include "apptoolbox.h"
#include "guiapplication.h"
#include "project/projecteditor.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/editor/graphics/graphicslayer.h>
#include <librepcb/editor/project/projecteditor.h>
#include <librepcb/editor/project/schematiceditor/fsm/schematiceditorfsm.h>
#include <librepcb/editor/project/schematiceditor/schematiceditor.h>
#include <librepcb/editor/project/schematiceditor/schematicgraphicsscene.h>
#include <librepcb/editor/utils/toolbarproxy.h>
#include <librepcb/editor/widgets/graphicsview.h>

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

static QString getTitle(std::shared_ptr<ProjectEditor> prj,
                        int schematicIndex) {
  if (auto s = prj->getProject().getSchematicByIndex(schematicIndex)) {
    return *s->getName();
  }
  return QString();
}

SchematicTab::SchematicTab(GuiApplication& app,
                           std::shared_ptr<ProjectEditor> prj,
                           int schematicIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, ui::TabType::Schematic, prj, schematicIndex,
                     getTitle(prj, schematicIndex), Qt::white, parent),
    mUiData{
        q2s(mBackgroundColor),  // Background color
        q2s(Qt::black),  // Overlay color
        true,  // Show pin numbers
        ui::SchematicTool::Select,  // Active tool
    },
    mFsm() {
  // Build the whole schematic editor finite state machine.
  auto editor = new editor::ProjectEditor(mApp.getWorkspace(),
                                          mProject->getProject(), std::nullopt);
  SchematicEditorFsm::Context fsmContext{
      mApp.getWorkspace(),
      mProject->getProject(),
      *editor,
      *new editor::SchematicEditor(*editor, mProject->getProject()),
      mScene,
      *new GraphicsView(),
      *new ToolBarProxy(),
      editor->getUndoStack()};
  mFsm.reset(new SchematicEditorFsm(fsmContext));
  // connect(mFsm.data(), &SchematicEditorFsm::statusBarMessageChanged, this,
  //         [this](const QString& message, int timeoutMs) {
  //           if (timeoutMs < 0) {
  //             mUi->statusbar->setPermanentMessage(message);
  //           } else {
  //             mUi->statusbar->showMessage(message, timeoutMs);
  //           }
  //         });
}

SchematicTab::~SchematicTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicTab::setUiData(const ui::SchematicTabData& data) noexcept {
  mUiData = data;

  if (auto l = mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers)) {
    l->setVisible(mUiData.show_pin_numbers);
  }

  emit requestRepaint();
}

void SchematicTab::activate() noexcept {
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    mScene.reset(new SchematicGraphicsScene(
        *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    mUiData.overlay_color = q2s(Qt::black);
    emit requestRepaint();
  }
}

void SchematicTab::deactivate() noexcept {
  mScene.reset();
}

bool SchematicTab::processSceneDoubleClicked(float x, float y, float width,
                                             float height) noexcept {
  Q_UNUSED(width);
  Q_UNUSED(height);

  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  const QPointF scenePosPx = tf.map(QPointF(x, y));

  QGraphicsSceneMouseEvent qe;
  qe.setScenePos(scenePosPx);
  if (auto win = qApp->activeWindow()) {
    qe.setScreenPos(win->mapToGlobal(QPointF(x, y)).toPoint());
  }

  bool handled = mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(qe);

  qDebug() << handled;

  if (handled) {
    emit requestRepaint();
  }

  return handled;
}

bool SchematicTab::processScenePointerEvent(
    float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  if (GraphicsSceneTab::processScenePointerEvent(x, y, width, height, e)) {
    return true;
  }

  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  const QPointF scenePosPx = tf.map(QPointF(x, y));

  QGraphicsSceneMouseEvent qe;
  qe.setScenePos(scenePosPx);
  if (auto win = qApp->activeWindow()) {
    qe.setScreenPos(win->mapToGlobal(QPointF(x, y)).toPoint());
  }

  bool handled = false;
  if ((e.button == slint::private_api::PointerEventButton::Left) &&
      (e.kind == slint::private_api::PointerEventKind::Down)) {
    qe.setButton(Qt::LeftButton);
    handled = mFsm->processGraphicsSceneLeftMouseButtonPressed(qe);
  } else if ((e.button == slint::private_api::PointerEventButton::Left) &&
             (e.kind == slint::private_api::PointerEventKind::Up)) {
    qe.setButton(Qt::LeftButton);
    handled = mFsm->processGraphicsSceneLeftMouseButtonReleased(qe);
  } else if ((e.button == slint::private_api::PointerEventButton::Right) &&
             (e.kind == slint::private_api::PointerEventKind::Up)) {
    qe.setButton(Qt::RightButton);
    handled = mFsm->processGraphicsSceneRightMouseButtonReleased(qe);
  } else if (e.kind == slint::private_api::PointerEventKind::Move) {
    handled = mFsm->processGraphicsSceneMouseMoved(qe);
  }

  if (handled) {
    emit requestRepaint();
  }

  return handled;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
