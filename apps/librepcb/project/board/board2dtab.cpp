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
#include "board2dtab.h"

#include "../../apptoolbox.h"
#include "../../guiapplication.h"
#include "../../notification.h"
#include "../../notificationsmodel.h"
#include "../../uitypes.h"
#include "../projecteditor.h"
#include "../projectsmodel.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/project/boardeditor/boardgraphicsscene.h>
#include <librepcb/editor/undostack.h>

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

Board2dTab::Board2dTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                       int boardIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, prj, boardIndex, parent),
    mEditor(prj),
    mDrc(new BoardDesignRuleCheck(this)),
    mDrcNotification(new Notification(ui::NotificationType::Progress, QString(),
                                      QString(), QString(), QString(), true)),
    mDrcUndoStackState(mEditor->getUndoStack().getUniqueStateId()),
    mDrcMessages() {
  // Apply settings from board.
  if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
    mGridInterval = brd->getGridInterval();
  }

  // Connect DRC.
  connect(mDrc.get(), &BoardDesignRuleCheck::progressPercent,
          mDrcNotification.get(), &Notification::setProgress);
  connect(mDrc.get(), &BoardDesignRuleCheck::progressStatus,
          mDrcNotification.get(), &Notification::setDescription);
  connect(mDrc.get(), &BoardDesignRuleCheck::finished, this,
          &Board2dTab::setDrcResult);

  // Connect undo stack.
  connect(&prj->getUndoStack(), &UndoStack::stateModified, this, [this]() {
    if (mDrcMessages) {
      emit uiDataChanged();
    }
  });
  connect(&prj->getUndoStack(), &UndoStack::stateModified, this,
          &Board2dTab::requestRepaint);
  connect(mEditor.get(), &ProjectEditor::manualModificationsMade, this,
          &Board2dTab::uiDataChanged);

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this,
          &Board2dTab::updateTheme);
  updateTheme();
}

Board2dTab::~Board2dTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData Board2dTab::getBaseUiData() const noexcept {
  auto brd = mProject->getProject().getBoardByIndex(mObjIndex);

  ui::RuleCheckState drcState;
  if (mDrc->isRunning()) {
    drcState = ui::RuleCheckState::Running;
  } else if (!mDrcMessages) {
    drcState = ui::RuleCheckState::NotRunYet;
  } else if (mDrcUndoStackState == mEditor->getUndoStack().getUniqueStateId()) {
    drcState = ui::RuleCheckState::UpToDate;
  } else {
    drcState = ui::RuleCheckState::Outdated;
  }

  return ui::TabData{
      ui::TabType::Board2d,  // Type
      q2s(brd ? *brd->getName() : QString()),  // Title
      q2s(QPixmap(":/projects.png")),  // Icon
      mApp.getProjects().getIndexOf(mEditor),  // Project index
      drcState,  // Rule check state
      mDrcMessages,  // Rule check messages
      mEditor->canSave(),  // Can save
      true,  // Can export graphics
      mProject->getUndoStack().canUndo(),  // Can undo
      q2s(mProject->getUndoStack().getUndoCmdText()),  // Undo text
      mProject->getUndoStack().canRedo(),  // Can redo
      q2s(mProject->getUndoStack().getRedoCmdText()),  // Redo text
      true,  // Can cut/copy
      true,  // Can paste
      true,  // Can remove
      true,  // Can rotate
      true,  // Can mirror
  };
}

ui::Board2dTabData Board2dTab::getUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  auto brd = mProject->getProject().getBoardByIndex(mObjIndex);

  QString gridIntervalStr;
  if (brd) {
    const LengthUnit& unit = brd->getGridUnit();
    gridIntervalStr = Toolbox::floatToString(unit.convertToUnit(*mGridInterval),
                                             10, QLocale());
  }

  return ui::Board2dTabData{
      q2s(mBackgroundColor),  // Background color
      q2s(theme.getColor(Theme::Color::sBoardOverlays)
              .getSecondaryColor()),  // Overlay color
      l2s(mGridStyle),  // Grid style
      q2s(gridIntervalStr),  // Grid interval
      brd ? l2s(brd->getGridUnit())
          : ui::LengthUnit::Millimeters,  // Length unit
  };
}

void Board2dTab::setUiData(const ui::Board2dTabData& data) noexcept {
  auto brd = mProject->getProject().getBoardByIndex(mObjIndex);

  mGridStyle = s2l(data.grid_style);
  const LengthUnit unit = s2l(data.unit);
  if (brd && (unit != brd->getGridUnit())) {
    brd->setGridUnit(unit);
    mEditor->setManualModificationsMade();
  }

  invalidateBackground();
  emit requestRepaint();
}

void Board2dTab::activate() noexcept {
  if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
    mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
    connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
            [this](BoardPlaneFragmentsBuilder::Result result) {
              if (result.applyToBoard()) {
                emit requestRepaint();
              }
            });
    mPlaneBuilder->start(*brd);
    mScene.reset(new BoardGraphicsScene(
        *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    emit requestRepaint();
  }
}

void Board2dTab::deactivate() noexcept {
  mPlaneBuilder.reset();
  mScene.reset();
}

bool Board2dTab::actionTriggered(ui::ActionId id) noexcept {
  if (id == ui::ActionId::Save) {
    mEditor->saveProject();
    return true;
  } else if (id == ui::ActionId::SectionGridIntervalIncrease) {
    mGridInterval = PositiveLength(mGridInterval * 2);
    invalidateBackground();
    return true;
  } else if ((id == ui::ActionId::SectionGridIntervalDecrease) &&
             ((*mGridInterval % 2) == 0)) {
    mGridInterval = PositiveLength(mGridInterval / 2);
    invalidateBackground();
    return true;
  } else if (id == ui::ActionId::RunQuickCheck) {
    startDrc(true);
    return true;
  } else if (id == ui::ActionId::RunDrc) {
    startDrc(false);
    return true;
  }

  return GraphicsSceneTab::actionTriggered(id);
}

const LengthUnit* Board2dTab::getCurrentUnit() const noexcept {
  if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
    return &brd->getGridUnit();
  } else {
    return nullptr;
  }
}

void Board2dTab::startDrc(bool quick) noexcept {
  auto board = mProject->getProject().getBoardByIndex(mObjIndex);
  if (!board) return;

  // Abort any ongoing run.
  mDrc->cancel();

  // Show progress notification during the run.
  mDrcNotification->setTitle(
      (quick ? tr("Running Quick Check") : tr("Running Design Rule Check")) %
      "...");
  mApp.getNotifications().add(mDrcNotification);

  // Run the DRC.
  mDrcUndoStackState = mEditor->getUndoStack().getUniqueStateId();
  mDrc->start(*board, board->getDrcSettings(), quick);  // can throw
  emit uiDataChanged();
}

void Board2dTab::setDrcResult(
    const BoardDesignRuleCheck::Result& result) noexcept {
  // TODO: Handle errors.

  auto board = mProject->getProject().getBoardByIndex(mObjIndex);

  // Detect & remove disappeared messages.
  const QSet<SExpression> approvals =
      RuleCheckMessage::getAllApprovals(result.messages);
  if (board && board->updateDrcMessageApprovals(approvals, result.quick)) {
    mEditor->setManualModificationsMade();
  }

  // Update UI.
  if (!mDrcMessages) {
    mDrcMessages.reset(new slint::VectorModel<ui::RuleCheckMessageData>());
  }
  l2s(result.messages,
      board ? board->getDrcMessageApprovals() : QSet<SExpression>{},
      *mDrcMessages);
  mDrcNotification->dismiss();
  emit uiDataChanged();
}

void Board2dTab::updateTheme() noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();

  mBackgroundColor =
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor();
  mGridColor =
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor();
  mGridStyle = theme.getBoardGridStyle();

  invalidateBackground();
  emit uiDataChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
