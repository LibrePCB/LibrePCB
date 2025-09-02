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

#include "../../3d/openglscenebuilder.h"
#include "../../3d/slintopenglview.h"
#include "../../guiapplication.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../projecteditor.h"
#include "boardeditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Board3dTab::Board3dTab(GuiApplication& app, BoardEditor& editor,
                       QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mProjectEditor(editor.getProjectEditor()),
    mProject(mProjectEditor.getProject()),
    mBoardEditor(editor),
    mBoard(mBoardEditor.getBoard()),
    mProjection(new OpenGlProjection()),
    mTimestampOfLastSceneRebuild(0),
    mFrameIndex(0) {
  Q_ASSERT(&mBoard.getProject() == &mProject);

  // Connect board editor.
  connect(&mBoardEditor, &BoardEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mBoardEditor, &BoardEditor::planesRebuildStatusChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mBoardEditor, &BoardEditor::planesUpdated, this,
          &Board3dTab::scheduleSceneRebuild);
  connect(&mBoardEditor, &BoardEditor::aboutToBeDestroyed, this,
          &Board3dTab::closeEnforced);

  // Connect project editor.
  connect(&mProjectEditor, &ProjectEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });

  // Connect undo stack.
  connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified, this,
          [this]() {
            if (!mProjectEditor.getUndoStack().isCommandGroupActive()) {
              scheduleSceneRebuild();
            }
            onUiDataChanged.notify();
          });
  connect(&mProjectEditor, &ProjectEditor::manualModificationsMade, this,
          [this]() { onUiDataChanged.notify(); });

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this, &Board3dTab::applyTheme);
  applyTheme();
}

Board3dTab::~Board3dTab() noexcept {
  deactivate();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int Board3dTab::getProjectIndex() const noexcept {
  return mProjectEditor.getUiIndex();
}

int Board3dTab::getProjectObjectIndex() const noexcept {
  return mProject.getBoardIndex(mBoard);
}

ui::TabData Board3dTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.save = toFs(mProject.getDirectory().isWritable());
  features.undo = toFs(mProjectEditor.getUndoStack().canUndo());
  features.redo = toFs(mProjectEditor.getUndoStack().canRedo());
  features.zoom = toFs(true);

  return ui::TabData{
      ui::TabType::Board3d,  // Type
      q2s(*mBoard.getName()),  // Title
      features,  // Features
      !mProject.getDirectory().isWritable(),  // Read-only
      mProjectEditor.hasUnsavedChanges(),  // Unsaved changes
      q2s(mProjectEditor.getUndoStack().getUndoCmdText()),  // Undo text
      q2s(mProjectEditor.getUndoStack().getRedoCmdText()),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::Board3dTabData Board3dTab::getDerivedUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  const QColor bgColor =
      theme.getColor(Theme::Color::s3dBackground).getPrimaryColor();
  const QColor fgColor =
      theme.getColor(Theme::Color::s3dBackground).getSecondaryColor();

  const bool refreshing = mBoardEditor.isRebuildingPlanes() ||
      (mSceneBuilder && mSceneBuilder->isBusy());
  QStringList errors = mSceneBuilderErrors;
  if (mView) errors += mView->getOpenGlErrors();

  return ui::Board3dTabData{
      mProjectEditor.getUiIndex(),  // Project index
      mBoardEditor.getUiIndex(),  // Board index
      q2s(bgColor),  // Background color
      q2s(fgColor),  // Foreground color
      q2s((mView && mView->isPanning()) ? Qt::ClosedHandCursor
                                        : Qt::ArrowCursor),  // Cursor
      mAlpha.value(OpenGlObject::Type::SolderResist, 1),  // Solder resist alpha
      mAlpha.value(OpenGlObject::Type::Silkscreen, 1),  // Silkscreen alpha
      mAlpha.value(OpenGlObject::Type::SolderPaste, 1),  // Solder paste alpha
      mAlpha.value(OpenGlObject::Type::Device, 1),
      refreshing,  // Refreshing
      q2s(errors.join("\n\n")),  // Error
      mFrameIndex,  // Frame index
  };
}

void Board3dTab::setDerivedUiData(const ui::Board3dTabData& data) noexcept {
  mAlpha[OpenGlObject::Type::SolderResist] =
      qBound(0.0f, data.solderresist_alpha, 1.0f);
  mAlpha[OpenGlObject::Type::Silkscreen] =
      qBound(0.0f, data.silkscreen_alpha, 1.0f);
  mAlpha[OpenGlObject::Type::SolderPaste] =
      qBound(0.0f, data.solderpaste_alpha, 1.0f);
  mAlpha[OpenGlObject::Type::Device] = qBound(0.0f, data.devices_alpha, 1.0f);
  if (mView) {
    mView->setAlpha(mAlpha);
  }

  requestRepaint();
}

void Board3dTab::activate() noexcept {
  if (!mView) {
    mView.reset(new SlintOpenGlView(*mProjection));
    mView->setAlpha(mAlpha);
    connect(mView.get(), &SlintOpenGlView::stateChanged, this,
            [this]() { onDerivedUiDataChanged.notify(); });
    connect(mView.get(), &SlintOpenGlView::contentChanged, this,
            &Board3dTab::requestRepaint);
  }

  if (!mSceneBuilder) {
    mSceneBuilder.reset(new OpenGlSceneBuilder(this));
    connect(mSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded, mView.get(),
            &SlintOpenGlView::addObject);
    connect(mSceneBuilder.get(), &OpenGlSceneBuilder::objectRemoved,
            mView.get(), &SlintOpenGlView::removeObject);
    connect(mSceneBuilder.get(), &OpenGlSceneBuilder::finished, this,
            [this](QStringList errors) {
              mSceneBuilderErrors = errors;
              mTimestampOfLastSceneRebuild =
                  QDateTime::currentMSecsSinceEpoch();
              onDerivedUiDataChanged.notify();
            });
  }

  // Setup timer for automatic scene rebuild.
  mSceneRebuildTimer.reset(new QTimer(this));
  mSceneRebuildTimer->setInterval(150);
  connect(mSceneRebuildTimer.get(), &QTimer::timeout, this,
          &Board3dTab::sceneRebuildTimerTimeout);
  scheduleSceneRebuild();

  applyTheme();
  mBoardEditor.registerActiveTab(this);
  requestRepaint();
}

void Board3dTab::deactivate() noexcept {
  if (mView) {
    *mProjection = mView->getProjection();
    mAlpha = mView->getAlpha();
  }
  mSceneRebuildTimer.reset();
  mBoardEditor.unregisterActiveTab(this);

  // We could reset the view here to release memory. But it leads to a (possibly
  // expensive/slow) scene rebuild when switching to this tab again, which is
  // a bit annoying. In future we may implement some memory management which
  // releases the OpenGL view when many tabs are opened or when this tab is
  // not shown for a long time.
  // mSceneBuilder.reset();
  // mView.reset();
}

void Board3dTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::BillOfMaterials: {
      mProjectEditor.execBomReviewDialog(&mBoard);
      break;
    }
    case ui::TabAction::Save: {
      mProjectEditor.saveProject();
      break;
    }
    case ui::TabAction::Undo: {
      mProjectEditor.undo();
      break;
    }
    case ui::TabAction::Redo: {
      mProjectEditor.redo();
      break;
    }
    case ui::TabAction::ZoomIn: {
      if (mView) mView->zoomIn();
      break;
    }
    case ui::TabAction::ZoomOut: {
      if (mView) mView->zoomOut();
      break;
    }
    case ui::TabAction::ZoomFit: {
      if (mView) mView->zoomAll();
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

slint::Image Board3dTab::renderScene(float width, float height,
                                     int scene) noexcept {
  Q_UNUSED(scene);
  if (mView) {
    return mView->render(width, height);
  }
  return slint::Image();
}

bool Board3dTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  return mView ? mView->pointerEvent(pos, e) : false;
}

bool Board3dTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  return mView ? mView->scrollEvent(pos, e) : false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Board3dTab::scheduleSceneRebuild() noexcept {
  if (mSceneRebuildTimer) mSceneRebuildTimer->start();
}

void Board3dTab::sceneRebuildTimerTimeout() noexcept {
  if ((!mView) || (!mSceneBuilder) || mSceneBuilder->isBusy()) {
    return;
  }
  if (mProjectEditor.getUndoStack().isCommandGroupActive() ||
      mBoardEditor.isRebuildingPlanes()) {
    return;
  }
  const qint64 pauseMs =
      QDateTime::currentMSecsSinceEpoch() - mTimestampOfLastSceneRebuild;
  if (pauseMs < 1000) {
    return;
  }

  if (mSceneRebuildTimer) {
    mSceneRebuildTimer->stop();
  }

  auto av = mProject.getCircuit().getAssemblyVariants().value(0);
  mSceneBuilder->start(mBoard.buildScene3D(
      av ? std::make_optional(av->getUuid()) : std::nullopt));

  onUiDataChanged.notify();
}

void Board3dTab::applyTheme() noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();

  if (mView) {
    mView->setBackgroundColor(
        theme.getColor(Theme::Color::s3dBackground).getPrimaryColor());
  }

  onDerivedUiDataChanged.notify();
}

void Board3dTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
