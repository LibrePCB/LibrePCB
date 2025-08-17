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
#include "../guiapplication.h"
#include "../notification.h"
#include "../notificationsmodel.h"
#include "../rulecheck/rulecheckmessagesmodel.h"
#include "../undostack.h"
#include "../utils/slinthelpers.h"
#include "../utils/standardeditorcommandhandler.h"
#include "../utils/uihelpers.h"
#include "board/boardeditor.h"
#include "bomreviewdialog.h"
#include "cmd/cmdboardadd.h"
#include "cmd/cmdboardremove.h"
#include "cmd/cmdschematicadd.h"
#include "cmd/cmdschematicedit.h"
#include "cmd/cmdschematicremove.h"
#include "outputjobsdialog/outputjobsdialog.h"
#include "projectsetupdialog.h"
#include "schematic/schematiceditor.h"
#include "schematic/schematictab.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguard.h>
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

ProjectEditor::ProjectEditor(
    GuiApplication& app, std::unique_ptr<Project> project, int uiIndex,
    const std::optional<QList<FileFormatMigration::Message>>& upgradeMessages,
    QObject* parent) noexcept
  : QObject(parent),
    onUiDataChanged(*this),
    mApp(app),
    mWorkspace(app.getWorkspace()),
    mProject(std::move(project)),
    mUiIndex(uiIndex),
    mUseIeee315Symbols(false),
    mUpgradeMessages(upgradeMessages),
    mSchematics(new UiObjectList<SchematicEditor, ui::SchematicData>()),
    mBoards(new UiObjectList<BoardEditor, ui::BoardData>()),
    mUndoStack(new UndoStack()),
    mHighlightedNetSignals(new QSet<const NetSignal*>()),
    mActiveSchematicTabs(),
    mErcMessages(),
    mErcExecutionError(),
    mManualModificationsMade(false),
    mLastAutosaveStateId(mUndoStack->getUniqueStateId()),
    mAutoSaveTimer() {
  // Populate schematics.
  auto updateSchematicIndices = [this]() {
    for (int i = 0; i < mSchematics->count(); ++i) {
      mSchematics->at(i)->setUiIndex(i);
    }
  };
  auto addSchematic = [this, updateSchematicIndices](int index) {
    auto sch = mProject->getSchematicByIndex(index);
    if (mSchematics && sch) {
      mSchematics->insert(
          index, std::make_shared<SchematicEditor>(*this, *sch, index));
      updateSchematicIndices();
    } else {
      qCritical() << "ProjectEditor: Invalid schematic index!";
    }
  };
  for (int i = 0; i < mProject->getSchematics().count(); ++i) {
    addSchematic(i);
  }
  connect(mProject.get(), &Project::schematicAdded, this, addSchematic);
  connect(mProject.get(), &Project::schematicRemoved, this,
          [this, updateSchematicIndices](int index) {
            if (mSchematics) {
              mSchematics->remove(index);
              updateSchematicIndices();
            }
          });

  // Populate boards.
  auto updateBoardIndices = [this]() {
    for (int i = 0; i < mBoards->count(); ++i) {
      mBoards->at(i)->setUiIndex(i);
    }
  };
  auto addBoard = [this, updateBoardIndices](int index) {
    auto brd = mProject->getBoardByIndex(index);
    if (mBoards && brd) {
      mBoards->insert(index, std::make_shared<BoardEditor>(*this, *brd, index));
      updateBoardIndices();
    } else {
      qCritical() << "ProjectEditor: Invalid board index!";
    }
  };
  for (int i = 0; i < mProject->getBoards().count(); ++i) {
    addBoard(i);
  }
  connect(mProject.get(), &Project::boardAdded, this, addBoard);
  connect(mProject.get(), &Project::boardRemoved, this,
          [this, updateBoardIndices](int index) {
            if (mBoards) {
              mBoards->remove(index);
              updateBoardIndices();
            }
          });

  // Show notification if file format has been upgraded.
  if (mUpgradeMessages) {
    QString msg =
        tr("The project '%1' has been upgraded to a new file format. "
           "After saving, it will not be possible anymore to open it with an "
           "older LibrePCB version!")
            .arg(*mProject->getName() % " " % mProject->getVersion());
    if (!mUpgradeMessages->isEmpty()) {
      msg += "\n\n" %
          tr("The upgrade produced %n message(s), please review before "
             "proceeding.",
             nullptr, mUpgradeMessages->count());
    }
    auto notification = std::make_shared<Notification>(
        ui::NotificationType::Warning,
        tr("ATTENTION: Project File Format Upgraded"), msg,
        (!mUpgradeMessages->isEmpty()) ? tr("Show Messages") : QString(),
        QString(), true);
    connect(notification.get(), &Notification::buttonClicked, this,
            &ProjectEditor::showUpgradeMessages);
    connect(this, &ProjectEditor::projectSavedToDisk, notification.get(),
            &Notification::dismiss);
    connect(this, &ProjectEditor::destroyed, notification.get(),
            &Notification::dismiss);
    mApp.getNotifications().push(notification);
  }

  // Connect to project settings change.
  connect(mProject.get(), &Project::normOrderChanged, this,
          &ProjectEditor::projectSettingsChanged);
  projectSettingsChanged();

  // Connect to undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this, [this]() {
    scheduleErcRun();
    onUiDataChanged.notify();
    emit ercMarkersInvalidated();
  });

  // Setup delay timer for ERC to avoid extensive CPU load.
  mErcTimer.setSingleShot(true);
  connect(&mErcTimer, &QTimer::timeout, this, &ProjectEditor::runErc);
  scheduleErcRun();

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
  // Stop timers.
  mAutoSaveTimer.stop();
  mErcTimer.stop();

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();

  // Delete objects to avoid issues with still connected signal/slots.
  mHighlightedNetSignals->clear();
  if (mErcMessages) {
    mErcMessages->clear();
    mErcMessages.reset();
  }
  mProject.reset();  // This also closes schematic- & board editors.
  Q_ASSERT(mSchematics->isEmpty());
  Q_ASSERT(mBoards->isEmpty());
  Q_ASSERT(mActiveSchematicTabs.isEmpty());

  // Now after the editors are closed, we are save to delete the undo stack.
  // Must *not* be done earlier since the editors have references to this!
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectEditor::setUiIndex(int index) noexcept {
  if (index != mUiIndex) {
    mUiIndex = index;
    emit uiIndexChanged();
  }
}

ui::ProjectData ProjectEditor::getUiData() const noexcept {
  return ui::ProjectData{
      true,  // Valid
      q2s(mProject->getFilepath().toNative()),  // Path
      q2s(*mProject->getName()),  // Name
      mSchematics,  // Schematics
      mBoards,  // Boards
      mProject->getDirectory().isWritable(),  // Writable
      mUseIeee315Symbols,  // Use IEEE315 symbols
      mManualModificationsMade || (!mUndoStack->isClean()),  // Unsaved changes
      ui::RuleCheckData{
          ui::RuleCheckType::Erc,  // Type
          mErcMessages ? ui::RuleCheckState::UpToDate
                       : ui::RuleCheckState::NotRunYet,  // State
          mErcMessages,  // Messages
          mErcMessages ? mErcMessages->getUnapprovedCount() : 0,  // Unapproved
          mErcMessages ? mErcMessages->getErrorCount() : 0,  // Errors
          q2s(mErcExecutionError),  // Execution error
          !mProject->getDirectory().isWritable(),  // Read-only
      },
  };
}

void ProjectEditor::setUiData(const ui::ProjectData& data) noexcept {
  Q_UNUSED(data);
}

void ProjectEditor::trigger(ui::ProjectAction a) noexcept {
  switch (a) {
    case ui::ProjectAction::Save: {
      saveProject();
      break;
    }

    case ui::ProjectAction::BillOfMaterials: {
      execBomReviewDialog((mProject->getBoards().count() == 1)
                              ? mProject->getBoardByIndex(0)
                              : nullptr);
      break;
    }

    case ui::ProjectAction::ExportLppz: {
      execLppzExportDialog(qApp->activeWindow());
      break;
    }

    case ui::ProjectAction::OpenFolder: {
      StandardEditorCommandHandler handler(mWorkspace.getSettings(),
                                           qApp->activeWindow());
      handler.fileManager(mProject->getPath());
      break;
    }

    case ui::ProjectAction::OpenOutputJobs: {
      execOutputJobsDialog();
      break;
    }

    case ui::ProjectAction::OpenSetupDialog: {
      execSetupDialog();
      break;
    }

    case ui::ProjectAction::UpdateLibrary: {
      emit projectLibraryUpdaterRequested(mProject->getFilepath());
      break;
    }

    default: {
      qWarning() << "Unhandled action in ProjectEditor:" << static_cast<int>(a);
      break;
    }
  }
}

void ProjectEditor::setHighlightedNetSignals(
    const QSet<const NetSignal*>& netSignals) noexcept {
  if (netSignals != *mHighlightedNetSignals) {
    *mHighlightedNetSignals = netSignals;
    emit highlightedNetSignalsChanged();
  }
}

bool ProjectEditor::hasUnsavedChanges() const noexcept {
  // If the project was upgraded, show it as modified to make it clear that
  // saving the project will modify the files.
  return mManualModificationsMade || (!mUndoStack->isClean()) ||
      mUpgradeMessages;
}

void ProjectEditor::undo() noexcept {
  try {
    mUndoStack->undo();
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
}

void ProjectEditor::redo() noexcept {
  try {
    mUndoStack->redo();
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
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
      onUiDataChanged.notify();
      emit manualModificationsMade();
    }

    // Saving was successful --> clean the undo stack.
    mUndoStack->setClean();
    if (mUpgradeMessages) {
      mUpgradeMessages = std::nullopt;  // Not needed anymore.
      // It's a bit ugly, but if no changes were made to the project, the UI
      // remains in "modified" state so we manually emit the stateModified()
      // signal here to ensure it gets updated.
      emit mUndoStack->stateModified();
    }
    emit projectSavedToDisk();
    emit statusBarMessageChanged(tr("Project saved!"), 2000);
    qDebug() << "Successfully saved project.";
    onUiDataChanged.notify();
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
  if (!mManualModificationsMade) {
    mManualModificationsMade = true;
    onUiDataChanged.notify();
    emit manualModificationsMade();
  }
}

void ProjectEditor::execSetupDialog() noexcept {
  abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.
  ProjectSetupDialog dialog(*mProject, *mUndoStack, qApp->activeWindow());
  dialog.exec();
}

void ProjectEditor::execOutputJobsDialog(const QString& typeName) noexcept {
  abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.
  OutputJobsDialog dlg(mWorkspace.getSettings(), *mProject, *mUndoStack,
                       qApp->activeWindow());

  // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
  QMetaObject::invokeMethod(
      &dlg, [&dlg, typeName]() { dlg.preselectJobByType(typeName); },
      Qt::QueuedConnection);

  dlg.exec();
}

void ProjectEditor::execBomReviewDialog(const Board* board) noexcept {
  BomReviewDialog dialog(mWorkspace.getSettings(), *mProject, board,
                         qApp->activeWindow());
  connect(&dialog, &BomReviewDialog::projectSettingsModified, this,
          &ProjectEditor::setManualModificationsMade);
  dialog.exec();
}

void ProjectEditor::execLppzExportDialog(QWidget* parent) noexcept {
  try {
    const FilePath defaultFp = mProject->getPath().getPathTo(
        mProject->getFilepath().getBasename() % ".lppz");
    QString filename = FileDialog::getSaveFileName(
        parent, tr("Export project to *.lppz"), defaultFp.toStr(), "*.lppz");
    if (filename.isEmpty()) return;
    if (!filename.endsWith(".lppz")) filename.append(".lppz");
    const FilePath fp(filename);
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
      mProject->save();  // can throw
    }

    // Export project to ZIP, but without the output directory since this can
    // be quite large and usually does not make sense, especially since *.lppz
    // files might even be stored in this directory as well because they are
    // output files.
    auto filter = [](const QString& filePath) {
      return !filePath.startsWith("output/");
    };
    mProject->getDirectory().getFileSystem()->exportToZip(fp,
                                                          filter);  // can throw
    emit statusBarMessageChanged(tr("Export succeeded!"), 2000);
    qDebug() << "Successfully exported project to *.lppz.";
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Error"), e.getMsg());
  }
}

std::shared_ptr<SchematicEditor> ProjectEditor::execNewSheetDialog() noexcept {
  QString name = tr("Sheet %1").arg(mProject->getSchematics().count() + 1);
  if (!ElementNameConstraint()(name)) {
    name = QString("Sheet %1").arg(mProject->getSchematics().count() + 1);
  }

  bool ok = false;
  name = QInputDialog::getText(qApp->activeWindow(), tr("New Sheet"),
                               tr("Choose a name for the new schematic page:"),
                               QLineEdit::Normal, name, &ok);
  if (!ok) return nullptr;

  try {
    const QString dirName = FilePath::cleanFileName(
        name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (dirName.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Invalid name: '%1'").arg(name));
    }

    abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.
    const int index = mProject->getSchematics().count();
    CmdSchematicAdd* cmd = new CmdSchematicAdd(*mProject, dirName,
                                               ElementName(name));  // can throw
    mUndoStack->execCmd(cmd);  // can throw
    return mSchematics->value(index);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return nullptr;
  }
}

void ProjectEditor::execRenameSheetDialog(int index) noexcept {
  Schematic* schematic = mProject->getSchematicByIndex(index);
  if (!schematic) return;

  bool ok = false;
  const QString name = QInputDialog::getText(
      qApp->activeWindow(), tr("Rename sheet"), tr("Choose new name:"),
      QLineEdit::Normal, *schematic->getName(), &ok);
  if (!ok) return;

  abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.

  try {
    std::unique_ptr<CmdSchematicEdit> cmd(new CmdSchematicEdit(*schematic));
    cmd->setName(ElementName(cleanElementName(name)));  // can throw
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void ProjectEditor::execDeleteSheetDialog(int index) noexcept {
  Schematic* schematic = mProject->getSchematicByIndex(index);
  if (!schematic) return;

  abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.

  try {
    mUndoStack->execCmd(new CmdSchematicRemove(*mProject, *schematic));
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

std::shared_ptr<BoardEditor> ProjectEditor::execNewBoardDialog(
    std::optional<int> copyFromIndex) noexcept {
  const Board* copyFrom =
      copyFromIndex ? mProject->getBoardByIndex(*copyFromIndex) : nullptr;
  if (copyFromIndex && (!copyFrom)) {
    qWarning() << "ProjectEditor: Invalid board index to copy from.";
    return nullptr;
  }

  QString name = tr("Board %1").arg(mProject->getBoards().count() + 1);
  if (copyFrom) {
    name = tr("Copy of %1").arg(*copyFrom->getName());
    if (!ElementNameConstraint()(name)) {
      name = QString("Copy of %1").arg(*copyFrom->getName());
    }
  }
  if (!ElementNameConstraint()(name)) {
    name = QString("Board %1").arg(mProject->getBoards().count() + 1);
  }

  bool ok = false;
  name = QInputDialog::getText(
      qApp->activeWindow(), copyFrom ? tr("Copy Board") : tr("Add New Board"),
      tr("Choose a name:"), QLineEdit::Normal, name, &ok);
  if (!ok) return nullptr;

  abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.

  try {
    const QString dirName = FilePath::cleanFileName(
        name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (dirName.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Invalid name: '%1'").arg(name));
    }

    const int index = mProject->getBoards().count();
    CmdBoardAdd* cmd = new CmdBoardAdd(*mProject, dirName, ElementName(name),
                                       copyFrom);  // can throw
    mUndoStack->execCmd(cmd);
    return mBoards->value(index);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return nullptr;
  }
}

void ProjectEditor::execDeleteBoardDialog(int index) noexcept {
  Board* board = mProject->getBoardByIndex(index);
  if (!board) return;

  QMessageBox::StandardButton btn = QMessageBox::question(
      qApp->activeWindow(), tr("Remove board"),
      tr("Are you really sure to remove the board \"%1\"?")
          .arg(*board->getName()));
  if (btn != QMessageBox::Yes) return;

  abortBlockingToolsInOtherEditors(nullptr);  // Release undo stack.

  try {
    mUndoStack->execCmd(new CmdBoardRemove(*board));
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void ProjectEditor::registerActiveSchematicTab(SchematicTab* tab) noexcept {
  if (!mActiveSchematicTabs.contains(tab)) {
    mActiveSchematicTabs.append(tab);
    if (mActiveSchematicTabs.count() == 1) {
      scheduleErcRun();
    }
  }
}

void ProjectEditor::unregisterActiveSchematicTab(SchematicTab* tab) noexcept {
  mActiveSchematicTabs.removeOne(tab);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectEditor::showUpgradeMessages() noexcept {
  if (!mUpgradeMessages) return;

  std::sort(mUpgradeMessages->begin(), mUpgradeMessages->end(),
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
  QTableWidget* table = new QTableWidget(mUpgradeMessages->count(), 4, &dialog);
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
  for (int i = 0; i < mUpgradeMessages->count(); ++i) {
    const FileFormatMigration::Message m = mUpgradeMessages->at(i);
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

void ProjectEditor::scheduleErcRun() noexcept {
  mErcTimer.start(mActiveSchematicTabs.isEmpty() ? 1000 : 100);
}

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
      connect(mErcMessages.get(), &RuleCheckMessagesModel::highlightRequested,
              this, &ProjectEditor::ercMarkersInvalidated);
      connect(mErcMessages.get(), &RuleCheckMessagesModel::highlightRequested,
              this, &ProjectEditor::ercMessageHighlightRequested);
    }
    mErcMessages->setMessages(messages, approvals);
    mErcExecutionError.clear();

    qDebug() << "ERC succeeded after" << timer.elapsed() << "ms.";
  } catch (const Exception& e) {
    mErcExecutionError = e.getMsg();
    qCritical() << "ERC failed:" << e.getMsg();
  }

  onUiDataChanged.notify();
}

void ProjectEditor::projectSettingsChanged() noexcept {
  mUseIeee315Symbols = false;
  foreach (const QString& norm, mProject->getNormOrder()) {
    if (norm.toLower() == "ieee 315") {
      mUseIeee315Symbols = true;
      break;
    } else if (norm.toLower() == "iec 60617") {
      mUseIeee315Symbols = false;
      break;
    }
  }
  onUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
