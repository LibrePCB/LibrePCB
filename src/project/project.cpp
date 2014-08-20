/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "project.h"
#include "../common/exceptions.h"
#include "../common/xmlfile.h"
#include "../workspace/workspace.h"
#include "../workspace/workspacesettings.h"
#include "library/projectlibrary.h"
#include "circuit/circuit.h"
#include "schematics/schematiceditor.h"
#include "../common/systeminfo.h"
#include "../common/filelock.h"
#include "../common/filepath.h"
#include "../common/undostack.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Project::Project(Workspace& workspace, const FilePath& filepath) throw (Exception) :
    QObject(0), mWorkspace(workspace), mPath(filepath.getParentDir()),
    mFilepath(filepath), mXmlFile(0), mFileLock(filepath), mUndoStack(0),
    mProjectLibrary(0), mCircuit(0), mSchematicEditor(0)
{
    qDebug() << "load project...";

    // Check if the filepath is valid
    if ((!mFilepath.isExistingFile()) || (mFilepath.getSuffix() != "e4u")
            || (!mPath.isExistingDir()))
    {
        throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(),
            QString(tr("Invalid project file: \"%1\"")).arg(mFilepath.toNative()));
    }

    // Check if the project is locked (already open or application was crashed). In case
    // of a crash, the user can decide if the last backup should be restored. If the
    // project should be opened, the lock file will be created/updated here.
    bool restoreBackup = false;
    switch (mFileLock.getStatus())
    {
        case FileLock::Unlocked:
        {
            // nothing to do here (the project will be locked later)
            break;
        }

        case FileLock::Locked:
        {
            // the project is locked by another application instance
            throw RuntimeError(__FILE__, __LINE__, QString(), tr("The project is already "
                               "opened by another application instance or user!"));
            break; // just to be on the safe side...
        }

        case FileLock::StaleLock:
        {
            // the application crashed while this project was open! ask the user what to do
            QMessageBox::StandardButton btn = QMessageBox::question(0, tr("Restore Project?"),
                tr("It seems that the application was crashed while this project was open. "
                "Do you want to restore the last automatic backup?"), QMessageBox::Yes |
                QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
            switch (btn)
            {
                case QMessageBox::Yes: // open the project and restore the last backup
                    restoreBackup = true;
                    break;
                case QMessageBox::No: // open the project without restoring the last backup
                    restoreBackup = false;
                    break;
                case QMessageBox::Cancel: // abort opening the project
                default:
                    throw UserCanceled(__FILE__, __LINE__, "canceled by the user");
                    break; // just to be on the safe side...
            }
            break;
        }

        case FileLock::Error:
        default:
        {
            throw RuntimeError(__FILE__, __LINE__, QString(),
                               tr("Could not read the project lock file!"));
            break; // just to be on the safe side...
        }
    }

    // the project can be opened by this application, so we will lock the whole project
    if (!mFileLock.lock())
    {
        throw RuntimeError(__FILE__, __LINE__, mFileLock.getLockFilepath().toStr(),
            QString(tr("Error while locking the project!\nDo you have write permissions "
            "to the file \"%1\"?")).arg(mFileLock.getLockFilepath().toNative()));
    }


    // OK - the project is locked and can be opened!
    // Until this line, there was no memory allocated on the heap. But in the rest of the
    // constructor, a lot of object will be created on the heap. If an exception is
    // thrown somewhere, we must ensure that all the allocated memory gets freed.
    // This is done by a try/catch block. In the catch-block, all allocated memory will
    // be freed. Then the exception is rethrown to leave the constructor.

    try
    {
        // try to open the XML project file
        mXmlFile = new XmlFile(mFilepath, restoreBackup, "project");

        // the project seems to be ready to open, so we will create all needed objects
        mUndoStack = new UndoStack();
        mProjectLibrary = new ProjectLibrary(mWorkspace, *this, restoreBackup);
        mCircuit = new Circuit(mWorkspace, *this, restoreBackup);
        mSchematicEditor = new SchematicEditor(mWorkspace, *this);
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        delete mSchematicEditor;        mSchematicEditor = 0;
        delete mCircuit;                mCircuit = 0;
        delete mProjectLibrary;         mProjectLibrary = 0;
        delete mUndoStack;              mUndoStack = 0;
        delete mXmlFile;                mXmlFile = 0;
        throw; // ...and rethrow the exception
    }

    // project successfully opened! :-)

    // setup the timer for automatic backups, if enabled in the settings
    int intervalSecs =  mWorkspace.getSettings().getProjectAutosaveInterval();
    if (intervalSecs > 0)
    {
        // autosaving is enabled --> start the timer
        connect(&mAutoSaveTimer, SIGNAL(timeout()), this, SLOT(autosave()));
        mAutoSaveTimer.start(1000 * intervalSecs);
    }

    qDebug() << "project successfully loaded!";
}

Project::~Project() noexcept
{
    // inform the workspace that this project will get destroyed
    mWorkspace.unregisterOpenProject(this);

    // stop the autosave timer
    mAutoSaveTimer.stop();

    // delete all command objects in the undo stack (must be done before other important
    // objects are deleted, as undo command objects can hold pointers/references to them!)
    mUndoStack->clear();

    // free the allocated memory in the reverse order of their allocation
    delete mSchematicEditor;        mSchematicEditor = 0;
    delete mCircuit;                mCircuit = 0;
    delete mProjectLibrary;         mProjectLibrary = 0;
    delete mUndoStack;              mUndoStack = 0;
    delete mXmlFile;                mXmlFile = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Project::windowIsAboutToClose(QMainWindow* window)
{
    int countOfOpenWindows = 0;
    if (mSchematicEditor->isVisible())  {countOfOpenWindows++;}

    if (countOfOpenWindows <= 1)
    {
        // the last open window (schematic editor, board editor, ...) is about to close.
        // --> close the whole project
        return close(window);
    }

    return true; // this is not the last open window, so no problem to close it...
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void Project::showSchematicEditor()
{
    mSchematicEditor->show();
    mSchematicEditor->raise();
    mSchematicEditor->activateWindow();
}

bool Project::save() noexcept
{
    QStringList errors;

    // step 1: save whole project to temporary files
    qDebug() << "Begin saving the project to temporary files...";
    if (!save(false, errors))
    {
        QMessageBox::critical(0, tr("Error while saving the project"),
            QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n")));
        qCritical() << "Project saving (1) finished with" << errors.count() << "errors!";
        return false;
    }

    if (errors.count() > 0) // This should not happen! There must be an error in the code!
    {
        QMessageBox::critical(0, tr("Error while saving the project"),
            QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n")));
        qCritical() << "save() has returned true, but there are" << errors.count() << "errors!";
        return false;
    }

    // step 2: save whole project to original files
    qDebug() << "Begin saving the project to original files...";
    if (!save(true, errors))
    {
        QMessageBox::critical(0, tr("Error while saving the project"),
            QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n")));
        qCritical() << "Project saving (2) finished with" << errors.count() << "errors!";
        return false;
    }

    // saving to the original files was successful --> clean the undo stack
    mUndoStack->setClean();
    qDebug() << "Project successfully saved";
    return true;
}

bool Project::autosave() noexcept
{
    QStringList errors;

    qDebug() << "Autosave the project...";

    if (!save(false, errors))
    {
        qCritical() << "Project autosave finished with" << errors.count() << "errors!";
        return false;
    }

    qDebug() << "Project autosave was successful";
    return true;
}

bool Project::close(QWidget* msgBoxParent)
{
    if (mUndoStack->isClean())
    {
        // no unsaved changes --> the project can be closed
        deleteLater();  // this project object will be deleted later in the event loop
        return true;
    }

    QMessageBox::StandardButton choice = QMessageBox::question(msgBoxParent,
         tr("Save Project?"), tr("You have unsaved changes in the project.\n"
         "Do you want to save them bevore closing the project?"),
         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

    switch (choice)
    {
        case QMessageBox::Yes: // save and close project
            if (save())
            {
                deleteLater(); // this project object will be deleted later in the event loop
                return true;
            }
            else
                return false;

        case QMessageBox::No: // close project without saving
            deleteLater(); // this project object will be deleted later in the event loop
            return true;

        default: // cancel, don't close the project
            return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Project::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;
    QString tilde = toOriginal ? "" : "~";

    // Save *.e4u project file
    try
    {
        mXmlFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    // Save other components
    if (!mCircuit->save(toOriginal, errors))
        success = false;

    return success;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
