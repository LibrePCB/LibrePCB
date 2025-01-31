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
#include "projecteditor.h"

#include "../apptoolbox.h"
#include "../uitypes.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
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

ProjectEditor::ProjectEditor(Workspace& ws, std::unique_ptr<Project> project,
                             QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mProject(std::move(project)),
    mUndoStack(new UndoStack()),
    mErcMessages(new slint::VectorModel<ui::RuleCheckMessageData>()),
    mManualModificationsMade(false),
    mLastAutosaveStateId(mUndoStack->getUniqueStateId()),
    mAutoSaveTimer() {
  // Run the ERC after opening and after every modification.
  QTimer::singleShot(200, this, &ProjectEditor::runErc);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ProjectEditor::runErc);

  // Setup the timer for automatic backups, if enabled in the settings.
  auto setupAutoSaveTimer = [this]() {
    const int intervalSecs =
        mWorkspace.getSettings().projectAutosaveIntervalSeconds.get();
    if (intervalSecs > 0) {
      mAutoSaveTimer.setInterval(1000 * intervalSecs);
      if (!mAutoSaveTimer.isActive()) {
        mAutoSaveTimer.start();
      }
    } else {
      mAutoSaveTimer.stop();
    }
  };
  connect(&mWorkspace.getSettings().projectAutosaveIntervalSeconds,
          &WorkspaceSettingsItem::edited, this, setupAutoSaveTimer);
  connect(&mAutoSaveTimer, &QTimer::timeout, this,
          &ProjectEditor::autosaveProject);
  setupAutoSaveTimer();
}

ProjectEditor::~ProjectEditor() noexcept {
  // Stop the autosave timer.
  mAutoSaveTimer.stop();

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool ProjectEditor::canSave() const noexcept {
  return (mManualModificationsMade || (!mUndoStack->isClean())) &&
      mProject->getDirectory().isWritable();
}

bool ProjectEditor::saveProject() noexcept {
  try {
    // Show waiting cursor during operation for immediate feedback even though
    // the operation can take some time.
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    auto csg = scopeGuard([]() { QGuiApplication::restoreOverrideCursor(); });

    // Save project.
    qDebug() << "Save project...";
    emit projectAboutToBeSaved();
    mProject->save();  // can throw
    mProject->getDirectory().getFileSystem()->save();  // can throw
    mLastAutosaveStateId = mUndoStack->getUniqueStateId();
    if (mManualModificationsMade) {
      mManualModificationsMade = false;
      emit manualModificationsMade();
    }

    // Saving was successful --> clean the undo stack.
    mUndoStack->setClean();
    emit projectSavedToDisk();
    // emit showTemporaryStatusBarMessage(tr("Project saved!"), 2000);
    qDebug() << "Successfully saved project.";
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(),
                          tr("Error while saving the project"), e.getMsg());
    return false;
  }
}

bool ProjectEditor::autosaveProject() noexcept {
  // Do not save if there are no changes since the last (auto)save.
  // Note: mUndoStack->isClean() must not be considered here since the undo
  // stack might be reverted to clean state by undoing commands. In that case,
  // the last autosave backup would be outdated and lead to unexpected state
  // when restoring.
  if (mUndoStack->getUniqueStateId() == mLastAutosaveStateId) {
    return false;
  }

  // If the user is executing a command at the moment, so we should not save
  // now, so we try it a few seconds later instead...
  if (mUndoStack->isCommandGroupActive()) {
    QTimer::singleShot(10000, this, &ProjectEditor::autosaveProject);
    return false;
  }

  // If the project directory is not writable, we cannot autosave.
  if (!mProject->getDirectory().isWritable()) {
    qInfo() << "Project directory is not writable, skipping autosave.";
    return false;
  }

  try {
    qDebug() << "Autosave project...";
    emit projectAboutToBeSaved();
    mProject->save();  // can throw
    mProject->getDirectory().getFileSystem()->autosave();  // can throw
    mLastAutosaveStateId = mUndoStack->getUniqueStateId();
    qDebug() << "Successfully autosaved project.";
    return true;
  } catch (const Exception& e) {
    qWarning() << "Project autosave failed:" << e.getMsg();
    return false;
  }
}

void ProjectEditor::setManualModificationsMade() noexcept {
  const bool oldState = mManualModificationsMade;
  mManualModificationsMade = true;
  if (!oldState) {
    emit manualModificationsMade();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectEditor::runErc() noexcept {
  try {
    QElapsedTimer timer;
    timer.start();
    ElectricalRuleCheck erc(*mProject);
    const auto messages = erc.runChecks();

    // Detect disappeared messages & remove their approvals.
    QSet<SExpression> approvals = RuleCheckMessage::getAllApprovals(messages);
    mSupportedErcApprovals |= approvals;
    mDisappearedErcApprovals = mSupportedErcApprovals - approvals;
    approvals = mProject->getErcMessageApprovals() - mDisappearedErcApprovals;
    saveErcMessageApprovals(approvals);

    // Update UI.
    l2s(messages, approvals, *mErcMessages);

    qDebug() << "ERC succeeded after" << timer.elapsed() << "ms.";
  } catch (const Exception& e) {
    qCritical() << "ERC failed:" << e.getMsg();
  }
}

void ProjectEditor::saveErcMessageApprovals(
    const QSet<SExpression>& approvals) noexcept {
  if (mProject->setErcMessageApprovals(approvals)) {
    // setManualModificationsMade(); TODO
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
