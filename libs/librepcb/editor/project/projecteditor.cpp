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

#include "../dialogs/filedialog.h"
#include "../undostack.h"
#include "boardeditor/boardeditor.h"
#include "orderpcbdialog.h"
#include "schematiceditor/schematiceditor.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectEditor::ProjectEditor(
    Workspace& workspace, Project& project,
    const std::optional<QList<FileFormatMigration::Message>>& upgradeMessages)
  : QObject(nullptr),
    mWorkspace(workspace),
    mProject(project),
    mHighlightedNetSignals(new QSet<const NetSignal*>()),
    mUndoStack(nullptr),
    mSchematicEditor(nullptr),
    mBoardEditor(nullptr),
    mLastAutosaveStateId(0),
    mManualModificationsMade(false) {
  try {
    if (upgradeMessages) {
      mUpgradeMessages = *upgradeMessages;
      mUpgradeMessageLabelText =
          tr("ATTENTION: This project has been upgraded to a new file format. "
             "After saving, it will not be possible anymore to open it with an "
             "older LibrePCB version!");
      if (!upgradeMessages->isEmpty()) {
        mUpgradeMessageLabelText += " ";
        mUpgradeMessageLabelText +=
            tr("The upgrade produced <a href='%1'>%2 message(s)</a>, please "
               "review before proceeding.",
               nullptr, upgradeMessages->count())
                .arg("messages")
                .arg(upgradeMessages->count());
      }
    }

    mUndoStack = new UndoStack();
    mLastAutosaveStateId = mUndoStack->getUniqueStateId();

    // create the whole schematic/board editor GUI inclusive FSM and so on
    mSchematicEditor = new SchematicEditor(*this, mProject);
    mBoardEditor = new BoardEditor(*this, mProject);
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
    delete mBoardEditor;
    mBoardEditor = nullptr;
    delete mSchematicEditor;
    mSchematicEditor = nullptr;
    delete mUndoStack;
    mUndoStack = nullptr;
    throw;  // ...and rethrow the exception
  }

  // Run the ERC after opening and after every modification.
  QTimer::singleShot(200, this, &ProjectEditor::runErc);
  connect(mUndoStack, &UndoStack::stateModified, this, &ProjectEditor::runErc);

  // setup the timer for automatic backups, if enabled in the settings
  int intervalSecs =
      mWorkspace.getSettings().projectAutosaveIntervalSeconds.get();
  if ((intervalSecs > 0) && project.getDirectory().isWritable()) {
    // autosaving is enabled --> start the timer
    connect(&mAutoSaveTimer, &QTimer::timeout, this,
            &ProjectEditor::autosaveProject);
    mAutoSaveTimer.start(1000 * intervalSecs);
  }
}

ProjectEditor::~ProjectEditor() noexcept {
  // stop the autosave timer
  mAutoSaveTimer.stop();

  // abort all active commands!
  mSchematicEditor->abortAllCommands();
  mBoardEditor->abortAllCommands();
  Q_ASSERT(!mUndoStack->isCommandGroupActive());

  // delete all command objects in the undo stack (must be done before other
  // important objects are deleted, as undo command objects can hold
  // pointers/references to them!)
  mUndoStack->clear();

  // free the allocated memory in the reverse order of their allocation

  delete mBoardEditor;
  mBoardEditor = nullptr;
  delete mSchematicEditor;
  mSchematicEditor = nullptr;
  delete mUndoStack;
  mUndoStack = nullptr;

  // emit "project editor closed" signal
  emit projectEditorClosed();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const LengthUnit& ProjectEditor::getDefaultLengthUnit() const noexcept {
  return mWorkspace.getSettings().defaultLengthUnit.get();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectEditor::abortBlockingToolsInOtherEditors(QWidget* editor) noexcept {
  if (mUndoStack->isCommandGroupActive()) {
    if (mSchematicEditor && (editor != mSchematicEditor)) {
      mSchematicEditor->abortAllCommands();
    }
    if (mBoardEditor && (editor != mBoardEditor)) {
      mBoardEditor->abortAllCommands();
    }
  }
}

bool ProjectEditor::windowIsAboutToClose(QMainWindow& window) noexcept {
  if (getCountOfVisibleEditorWindows() > 1) {
    // this is not the last open window, so no problem to close it...
    return true;
  } else {
    // the last open window (schematic editor, board editor, ...) is about to
    // close.
    // --> close the whole project
    return closeAndDestroy(true, &window);
  }
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void ProjectEditor::showUpgradeMessages(QWidget* parent) noexcept {
  std::sort(mUpgradeMessages.begin(), mUpgradeMessages.end(),
            [](const FileFormatMigration::Message& a,
               const FileFormatMigration::Message& b) {
              if (a.severity > b.severity) return true;
              if (a.toVersion < b.toVersion) return true;
              if (a.message < b.message) return true;
              return false;
            });

  QDialog dialog(parent);
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

void ProjectEditor::showAllRequiredEditors() noexcept {
  // show board editor if there is at least one board
  if (!mProject.getBoards().isEmpty()) {
    showBoardEditor();
  }
  // show schematic editor if there is at least one schematic
  if (!mProject.getSchematics().isEmpty()) {
    showSchematicEditor();
  }
  // if there aren't any boards or schematics, show the schematic editor anyway
  if (getCountOfVisibleEditorWindows() < 1) {
    showSchematicEditor();
  }
  // verify if at least one editor window is now visible
  Q_ASSERT(getCountOfVisibleEditorWindows() > 0);
}

void ProjectEditor::showSchematicEditor() noexcept {
  mSchematicEditor->show();
  mSchematicEditor->raise();
  mSchematicEditor->activateWindow();
}

void ProjectEditor::showBoardEditor() noexcept {
  mBoardEditor->show();
  mBoardEditor->raise();
  mBoardEditor->activateWindow();
}

void ProjectEditor::execLppzExportDialog(QWidget* parent) noexcept {
  try {
    FilePath defaultFp = mProject.getPath().getPathTo(
        mProject.getFilepath().getBasename() % ".lppz");
    QString filename = FileDialog::getSaveFileName(
        parent, tr("Export project to *.lppz"), defaultFp.toStr(), "*.lppz");
    if (filename.isEmpty()) return;
    if (!filename.endsWith(".lppz")) filename.append(".lppz");
    FilePath fp(filename);
    qDebug().nospace() << "Export project to " << fp.toNative() << "...";

    // Usually we save the project to the transactional file system (but not to
    // the disk!) before exporting the *.lppz since the user probably expects
    // that the current state of the project gets exported. However, if the
    // file format is unstable (i.e. on development branches), this would lead
    // in a *.lppz of an unstable file format, which is not really useful (most
    // *.lppz readers will not support an unstable file format). Therefore we
    // don't save the project on development branches. Note that unfortunately
    // this doesn't work if there are any changes in the project and an autosave
    // was already performed, but it is almost impossible to fix this issue :-(
    if (Application::isFileFormatStable()) {
      mProject.save();  // can throw
    }

    // Export project to ZIP, but without the output directory since this can
    // be quite large and usually does not make sense, especially since *.lppz
    // files might even be stored in this directory as well because they are
    // output files.
    auto filter = [](const QString& filePath) {
      return !filePath.startsWith("output/");
    };
    mProject.getDirectory().getFileSystem()->exportToZip(fp,
                                                         filter);  // can throw
    qDebug() << "Successfully exported project to *.lppz.";
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Error"), e.getMsg());
  }
}

void ProjectEditor::execOrderPcbDialog(QWidget* parent) noexcept {
  auto callback = [this]() {
    if (Application::isFileFormatStable()) {
      // See explanation in execLppzExportDialog().
      mProject.save();  // can throw
    }
    // Export project to ZIP, but without the output directory since this can
    // be quite large and does not make sense to upload to the API server.
    auto filter = [](const QString& filePath) {
      return !filePath.startsWith("output/");
    };
    return mProject.getDirectory().getFileSystem()->exportToZip(
        filter);  // can throw
  };

  OrderPcbDialog dialog(mWorkspace.getSettings(), callback, parent);
  dialog.exec();
}

bool ProjectEditor::saveProject() noexcept {
  try {
    // SHow waiting cursor during operation for immediate feedback even though
    // the operation can take some time.
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    auto csg = scopeGuard([]() { QGuiApplication::restoreOverrideCursor(); });

    // Save project.
    qDebug() << "Save project...";
    emit projectAboutToBeSaved();
    mProject.save();  // can throw
    mProject.getDirectory().getFileSystem()->save();  // can throw
    mLastAutosaveStateId = mUndoStack->getUniqueStateId();
    mManualModificationsMade = false;

    // saving was successful --> clean the undo stack
    mUndoStack->setClean();
    emit projectSavedToDisk();
    emit showTemporaryStatusBarMessage(tr("Project saved!"), 2000);
    qDebug() << "Successfully saved project.";
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(nullptr, tr("Error while saving the project"),
                          e.getMsg());
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

  if (mUndoStack->isCommandGroupActive()) {
    // the user is executing a command at the moment, so we should not save now,
    // try it a few seconds later instead...
    QTimer::singleShot(10000, this, &ProjectEditor::autosaveProject);
    return false;
  }

  try {
    qDebug() << "Autosave project...";
    emit projectAboutToBeSaved();
    mProject.save();  // can throw
    mProject.getDirectory().getFileSystem()->autosave();  // can throw
    mLastAutosaveStateId = mUndoStack->getUniqueStateId();
    qDebug() << "Successfully autosaved project.";
    return true;
  } catch (Exception& exc) {
    return false;
  }
}

bool ProjectEditor::closeAndDestroy(bool askForSave,
                                    QWidget* msgBoxParent) noexcept {
  if ((mUndoStack->isClean() && (!mManualModificationsMade)) ||
      (!mProject.getDirectory().isWritable()) || (!askForSave)) {
    // no unsaved changes or opened in read-only mode or don't save --> close
    // project
    deleteLater();  // this project object will be deleted later in the event
                    // loop
    return true;
  }

  QMessageBox::StandardButton choice = QMessageBox::question(
      msgBoxParent, tr("Save Project?"),
      tr("You have unsaved changes in the project.\n"
         "Do you want to save them before closing the project?"),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Yes);

  switch (choice) {
    case QMessageBox::Yes:  // save and close project
      if (saveProject()) {
        deleteLater();  // this project object will be deleted later in the
                        // event loop
        return true;
      } else
        return false;

    case QMessageBox::No:  // close project without saving
      deleteLater();  // this project object will be deleted later in the event
                      // loop
      return true;

    default:  // cancel, don't close the project
      return false;
  }
}

void ProjectEditor::setErcMessageApproved(const RuleCheckMessage& msg,
                                          bool approve) noexcept {
  QSet<SExpression> approvals = mProject.getErcMessageApprovals();
  if (approve) {
    approvals.insert(msg.getApproval());
  } else {
    approvals.remove(msg.getApproval());
  }
  saveErcMessageApprovals(approvals);
}

void ProjectEditor::setHighlightedNetSignals(
    const QSet<const NetSignal*>& netSignals) noexcept {
  if (netSignals != *mHighlightedNetSignals) {
    *mHighlightedNetSignals = netSignals;
    emit highlightedNetSignalsChanged();
  }
}

void ProjectEditor::clearHighlightedNetSignals() noexcept {
  setHighlightedNetSignals({});
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectEditor::runErc() noexcept {
  try {
    QElapsedTimer timer;
    timer.start();
    ElectricalRuleCheck erc(mProject);
    mErcMessages = erc.runChecks();

    // Detect disappeared messages & remove their approvals.
    QSet<SExpression> approvals =
        RuleCheckMessage::getAllApprovals(mErcMessages);
    mSupportedErcApprovals |= approvals;
    mDisappearedErcApprovals = mSupportedErcApprovals - approvals;
    approvals = mProject.getErcMessageApprovals() - mDisappearedErcApprovals;
    saveErcMessageApprovals(approvals);

    emit ercFinished(mErcMessages);
    qDebug() << "ERC succeeded after" << timer.elapsed() << "ms.";
  } catch (const Exception& e) {
    qCritical() << "ERC failed:" << e.getMsg();
  }
}

void ProjectEditor::saveErcMessageApprovals(
    const QSet<SExpression>& approvals) noexcept {
  if (mProject.setErcMessageApprovals(approvals)) {
    setManualModificationsMade();
  }
}

int ProjectEditor::getCountOfVisibleEditorWindows() const noexcept {
  int count = 0;
  if (mSchematicEditor->isVisible()) {
    count++;
  }
  if (mBoardEditor->isVisible()) {
    count++;
  }
  return count;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
