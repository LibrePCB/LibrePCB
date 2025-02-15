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
#include "../guiapplication.h"
#include "../notification.h"
#include "../notificationsmodel.h"
#include "../rulecheck/rulecheckmessagesmodel.h"
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

ProjectEditor::ProjectEditor(
    GuiApplication& app, std::unique_ptr<Project> project,
    const std::optional<QList<FileFormatMigration::Message> >& upgradeMessages,
    QObject* parent) noexcept
  : QObject(parent),
    mApp(app),
    mWorkspace(app.getWorkspace()),
    mProject(std::move(project)),
    mUpgradeMessages(),
    mUndoStack(new UndoStack()),
    mErcMessages(),
    mErcExecutionError(),
    mManualModificationsMade(false),
    mLastAutosaveStateId(mUndoStack->getUniqueStateId()),
    mAutoSaveTimer() {
  // Show notification if file format has been upgraded.
  if (upgradeMessages) {
    mUpgradeMessages = *upgradeMessages;
    QString msg =
        tr("The project '%1' has been upgraded to a new file format. "
           "After saving, it will not be possible anymore to open it with an "
           "older LibrePCB version!")
            .arg(*mProject->getName() % " " % mProject->getVersion());
    if (!upgradeMessages->isEmpty()) {
      msg += "\n\n" %
          tr("The upgrade produced %n message(s), please review before "
             "proceeding.",
             nullptr, upgradeMessages->count());
    }
    auto notification = std::make_shared<Notification>(
        ui::NotificationType::Warning,
        tr("ATTENTION: Project File Format Upgraded"), msg,
        (!upgradeMessages->isEmpty()) ? tr("Show Messages") : QString(),
        QString(), true);
    connect(notification.get(), &Notification::buttonClicked, this,
            &ProjectEditor::showUpgradeMessages);
    connect(this, &ProjectEditor::projectSavedToDisk, notification.get(),
            &Notification::dismiss);
    mApp.getNotifications().add(notification);
  }

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

void ProjectEditor::showUpgradeMessages() noexcept {
  std::sort(mUpgradeMessages.begin(), mUpgradeMessages.end(),
            [](const FileFormatMigration::Message& a,
               const FileFormatMigration::Message& b) {
              if (a.severity > b.severity) return true;
              if (a.toVersion < b.toVersion) return true;
              if (a.message < b.message) return true;
              return false;
            });

  QDialog dialog(qApp->activeWindow());
  dialog.setWindowTitle(tr("File Format Upgrade Messages"));
  dialog.resize(800, 400);
  QVBoxLayout* layout = new QVBoxLayout(&dialog);
  QTableWidget* table = new QTableWidget(mUpgradeMessages.count(), 4, &dialog);
  table->setHorizontalHeaderLabels(
      {tr("Severity"), tr("Version"), tr("Occurrences"), tr("Message")});
  table->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  table->horizontalHeader()->setStretchLastSection(true);
  table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignLeft);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setWordWrap(true);
  for (int i = 0; i < mUpgradeMessages.count(); ++i) {
    const FileFormatMigration::Message m = mUpgradeMessages.at(i);
    QTableWidgetItem* item = new QTableWidgetItem(m.getSeverityStrTr());
    item->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 0, item);

    item = new QTableWidgetItem(m.fromVersion.toStr() % " → " %
                                m.toVersion.toStr());
    item->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 1, item);

    item = new QTableWidgetItem(
        (m.affectedItems > 0) ? QString::number(m.affectedItems) : QString());
    item->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 2, item);

    item = new QTableWidgetItem(m.message);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setItem(i, 3, item);
  }
  layout->addWidget(table);
  QTimer::singleShot(10, table, &QTableWidget::resizeRowsToContents);
  connect(table->horizontalHeader(), &QHeaderView::sectionResized, table,
          &QTableWidget::resizeRowsToContents);
  QDialogButtonBox* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::close);
  layout->addWidget(buttonBox);
  dialog.exec();
}

bool ProjectEditor::requestClose() noexcept {
  if ((mUndoStack->isClean() && (!mManualModificationsMade)) ||
      (!mProject->getDirectory().isWritable())) {
    // No unsaved changes or opened in read-only mode or don't save.
    return true;
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Project?"),
      tr("The project '%1' contains unsaved changes.\n"
         "Do you want to save them before closing the project?")
          .arg(*mProject->getName()),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Yes);
  if (choice == QMessageBox::Yes) {
    return saveProject();
  } else if (choice == QMessageBox::No) {
    return true;
  } else {
    return false;
  }
}

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
    if (mProject->setErcMessageApprovals(approvals)) {
      setManualModificationsMade();
    }

    // Update UI.
    if (!mErcMessages) {
      mErcMessages.reset(new RuleCheckMessagesModel());
      connect(mErcMessages.get(), &RuleCheckMessagesModel::approvalChanged,
              mProject.get(), &Project::setErcMessageApproved);
      connect(mErcMessages.get(), &RuleCheckMessagesModel::approvalChanged,
              this, &ProjectEditor::setManualModificationsMade);
    }
    mErcMessages->setMessages(messages, approvals);
    mErcExecutionError.clear();

    qDebug() << "ERC succeeded after" << timer.elapsed() << "ms.";
  } catch (const Exception& e) {
    mErcExecutionError = e.getMsg();
    qCritical() << "ERC failed:" << e.getMsg();
  }

  emit ercFinished();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
