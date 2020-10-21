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

#include "boardeditor/boardeditor.h"
#include "dialogs/editnetclassesdialog.h"
#include "dialogs/projectsettingsdialog.h"
#include "schematiceditor/schematiceditor.h"

#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/project.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectEditor::ProjectEditor(workspace::Workspace& workspace, Project& project)
  : QObject(nullptr),
    mWorkspace(workspace),
    mProject(project),
    mUndoStack(nullptr),
    mSchematicEditor(nullptr),
    mBoardEditor(nullptr) {
  try {
    mUndoStack = new UndoStack();

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

void ProjectEditor::execProjectSettingsDialog(QWidget* parent) noexcept {
  ProjectSettingsDialog d(mProject.getSettings(), *mUndoStack, parent);
  d.exec();
}

void ProjectEditor::execNetClassesEditorDialog(QWidget* parent) noexcept {
  EditNetClassesDialog d(mProject.getCircuit(), *mUndoStack, parent);
  d.exec();
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
    qDebug() << "Export project to *.lppz:" << fp.toNative();
    mProject.save();  // can throw
    mProject.getDirectory().getFileSystem()->exportToZip(fp);  // can throw
    qDebug() << "Project successfully exported.";
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Error"), e.getMsg());
  }
}

bool ProjectEditor::saveProject() noexcept {
  try {
    qDebug() << "Save project...";
    mProject.save();  // can throw
    mProject.getDirectory().getFileSystem()->save();  // can throw

    // saving was successful --> clean the undo stack
    mUndoStack->setClean();
    qDebug() << "Project successfully saved";
    return true;
  } catch (Exception& exc) {
    QMessageBox::critical(0, tr("Error while saving the project"),
                          exc.getMsg());
    return false;
  }
}

bool ProjectEditor::saveProjectAs(QWidget* parent) noexcept {
  try {
    qDebug() << "Saving project as...";

    FilePath defaultDirectory = mProject.getPath().getParentDir(); // can throw
    QString directoryName = FileDialog::getSaveFileName(
        parent, tr("Save project as *"), defaultDirectory.toStr(), "*");
    if (directoryName.isEmpty()) return false;

    // Copy project directory to 'directoryName' directory
    // create file system
    FilePath chosenDir = FilePath(directoryName);
    std::shared_ptr<TransactionalFileSystem> fs = TransactionalFileSystem::openRW(
      chosenDir);
    TransactionalDirectory transdir(fs);

    // Create a new project
    Project* project = Project::create(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
        directoryName);

    // Save project to its own transactional directory, and then copies it to the
    // newly created transactional directory
    mProject.save();
    mProject.getDirectory().copyTo(transdir);

    project->save();
    fs->save();

    // Do not clean the undo stack for saving as copy, could be useful?

    qDebug() << "Project successfully saved";
    return true;
  } catch (Exception& exc) {
      QMessageBox::critical(0, tr("Error while saving the project"),
                            exc.getMsg());
      return false;
    }
}

bool ProjectEditor::autosaveProject() noexcept {
  if (mUndoStack->isClean())
    return false;  // do not save if there are no changes

  if (mUndoStack->isCommandGroupActive()) {
    // the user is executing a command at the moment, so we should not save now,
    // try it a few seconds later instead...
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    QTimer::singleShot(10000, this, &ProjectEditor::autosaveProject);
#else
    QTimer::singleShot(10000, this, SLOT(autosaveProject()));
#endif
    return false;
  }

  try {
    qDebug() << "Autosave project...";
    mProject.save();  // can throw
    mProject.getDirectory().getFileSystem()->autosave();  // can throw
    qDebug() << "Project successfully autosaved";
    return true;
  } catch (Exception& exc) {
    return false;
  }
}

bool ProjectEditor::closeAndDestroy(bool askForSave,
                                    QWidget* msgBoxParent) noexcept {
  if (mUndoStack->isClean() || (!mProject.getDirectory().isWritable()) ||
      (!askForSave)) {
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

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
}  // namespace project
}  // namespace librepcb
