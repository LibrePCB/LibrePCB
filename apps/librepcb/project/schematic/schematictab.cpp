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

#include "../../apptoolbox.h"
#include "../../guiapplication.h"
#include "../../uitypes.h"
#include "../projecteditor.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
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
  : GraphicsSceneTab(app, ui::TabType::Schematic, QPixmap(":/image.svg"), prj,
                     schematicIndex, getTitle(prj, schematicIndex), parent),
    mFsm() {
  // Apply theme.
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  mBackgroundColor =
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor();
  mGridColor =
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor();
  mGridStyle = theme.getSchematicGridStyle();
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    mGridInterval = sch->getGridInterval();
  }

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
      prj->getUndoStack()};
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

ui::SchematicTabData SchematicTab::getUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  auto sch = mProject->getProject().getSchematicByIndex(mObjIndex);
  auto pinNumbersLayer =
      mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers);

  QString gridIntervalStr;
  if (sch) {
    const LengthUnit& unit = sch->getGridUnit();
    gridIntervalStr = Toolbox::floatToString(unit.convertToUnit(*mGridInterval),
                                             10, QLocale());
  }

  return ui::SchematicTabData{
      q2s(mBackgroundColor),  // Background color
      q2s(theme.getColor(Theme::Color::sSchematicBackground)
              .getSecondaryColor()),  // Overlay color
      l2s(mGridStyle),  // Grid style
      q2s(gridIntervalStr),  // Grid interval
      sch ? l2s(sch->getGridUnit())
          : ui::LengthUnit::Millimeters,  // Length unit
      pinNumbersLayer && pinNumbersLayer->isVisible(),  // Show pin numbers
      ui::SchematicTool::Select,  // Active tool
  };
}

void SchematicTab::setUiData(const ui::SchematicTabData& data) noexcept {
  mGridStyle = s2l(data.grid_style);
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    sch->setGridUnit(s2l(data.unit));
  }
  if (auto l = mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers)) {
    l->setVisible(data.show_pin_numbers);
  }

  invalidateBackground();
  emit requestRepaint();
}

void SchematicTab::activate() noexcept {
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    mScene.reset(new SchematicGraphicsScene(
        *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    emit requestRepaint();
  }
}

void SchematicTab::deactivate() noexcept {
  mScene.reset();
}

bool SchematicTab::actionTriggered(ui::ActionId id) noexcept {
  if (id == ui::ActionId::SectionGridIntervalIncrease) {
    mGridInterval = PositiveLength(mGridInterval * 2);
    invalidateBackground();
    return true;
  } else if ((id == ui::ActionId::SectionGridIntervalDecrease) &&
             ((*mGridInterval % 2) == 0)) {
    mGridInterval = PositiveLength(mGridInterval / 2);
    invalidateBackground();
    return true;
  }

  return GraphicsSceneTab::actionTriggered(id);
}

bool SchematicTab::processScenePointerEvent(
    const QPointF& pos, const QPointF& globalPos,
    slint::private_api::PointerEvent e) noexcept {
  static qint64 lastClickTime = 0;

  if (GraphicsSceneTab::processScenePointerEvent(pos, globalPos, e)) {
    return true;
  }

  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  const QPointF scenePosPx = tf.map(pos);

  QGraphicsSceneMouseEvent qe;
  qe.setScenePos(scenePosPx);
  qe.setScreenPos(globalPos.toPoint());

  bool isDoubleClick = false;
  if (e.kind == slint::private_api::PointerEventKind::Down) {
    const qint64 t = QDateTime::currentMSecsSinceEpoch();
    if (t - lastClickTime < 300) {
      isDoubleClick = true;
    }
    lastClickTime = t;
  }

  bool handled = false;
  if (isDoubleClick &&
      (e.button == slint::private_api::PointerEventButton::Left)) {
    qe.setButton(Qt::LeftButton);
    handled = mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(qe);
  } else if ((e.button == slint::private_api::PointerEventButton::Left) &&
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
