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
#include <QPrinter>
#include "project.h"
#include "../common/exceptions.h"
#include "../common/xmlfile.h"
#include "../common/inifile.h"
#include "../workspace/workspace.h"
#include "../workspace/settings/workspacesettings.h"
#include "library/projectlibrary.h"
#include "circuit/circuit.h"
#include "schematics/schematiceditor.h"
#include "../common/systeminfo.h"
#include "../common/filelock.h"
#include "../common/filepath.h"
#include "../common/undostack.h"
#include "schematics/schematic.h"
#include "../common/schematiclayer.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Project::Project(const FilePath& filepath, bool create) throw (Exception) :
    QObject(0), mPath(filepath.getParentDir()), mFilepath(filepath), mXmlFile(0),
    mFileLock(filepath), mIsRestored(false), mIsReadOnly(false), mSchematicsIniFile(0),
    mUndoStack(0), mProjectLibrary(0), mCircuit(0), mSchematicEditor(0)
{
    qDebug() << (create ? "create project..." : "open project...");

    // Check if the filepath is valid
    if (mFilepath.getSuffix() != "e4u")
    {
        throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(),
            tr("The suffix of the project file must be \"e4u\"!"));
    }
    if (create)
    {
        if (mFilepath.isExistingDir() || mFilepath.isExistingFile())
        {
            throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(), QString(tr(
                "The file \"%1\" does already exist!")).arg(mFilepath.toNative()));
        }
        if (!mPath.mkPath())
        {
            throw RuntimeError(__FILE__, __LINE__, mPath.toStr(), QString(tr(
                "Could not create the directory \"%1\"!")).arg(mPath.toNative()));
        }
    }
    else
    {
        if (((!mFilepath.isExistingFile())) || (!mPath.isExistingDir()))
        {
            throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(),
                QString(tr("Invalid project file: \"%1\"")).arg(mFilepath.toNative()));
        }
    }

    // Check if the project is locked (already open or application was crashed). In case
    // of a crash, the user can decide if the last backup should be restored. If the
    // project should be opened, the lock file will be created/updated here.
    switch (mFileLock.getStatus())
    {
        case FileLock::Unlocked:
        {
            // nothing to do here (the project will be locked later)
            break;
        }

        case FileLock::Locked:
        {
            // the project is locked by another application instance! open read only?
            QMessageBox::StandardButton btn = QMessageBox::question(0, tr("Open Read-Only?"),
                tr("The project is already opened by another application instance or user. "
                "Do you want to open the project in read-only mode?"), QMessageBox::Yes |
                QMessageBox::Cancel, QMessageBox::Cancel);
            switch (btn)
            {
                case QMessageBox::Yes: // open the project in read-only mode
                    mIsReadOnly = true;
                    break;
                default: // abort opening the project
                    throw UserCanceled(__FILE__, __LINE__);
            }
            break;
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
                    mIsRestored = true;
                    break;
                case QMessageBox::No: // open the project without restoring the last backup
                    mIsRestored = false;
                    break;
                default: // abort opening the project
                    throw UserCanceled(__FILE__, __LINE__);
            }
            break;
        }

        case FileLock::Error:
        default:
        {
            throw RuntimeError(__FILE__, __LINE__, QString(),
                               tr("Could not read the project lock file!"));
        }
    }

    // the project can be opened by this application, so we will lock the whole project
    if (!mIsReadOnly)
    {
        if (!mFileLock.lock())
        {
            throw RuntimeError(__FILE__, __LINE__, mFileLock.getLockFilepath().toStr(),
                QString(tr("Error while locking the project!\nDo you have write permissions "
                "to the file \"%1\"?")).arg(mFileLock.getLockFilepath().toNative()));
        }
    }

    // check if the combination of "create", "mIsRestored" and "mIsReadOnly" is valid
    Q_ASSERT(!(create && (mIsRestored || mIsReadOnly)));


    // OK - the project is locked (or read-only) and can be opened!
    // Until this line, there was no memory allocated on the heap. But in the rest of the
    // constructor, a lot of object will be created on the heap. If an exception is
    // thrown somewhere, we must ensure that all the allocated memory gets freed.
    // This is done by a try/catch block. In the catch-block, all allocated memory will
    // be freed. Then the exception is rethrown to leave the constructor.

    try
    {
        // try to create/open the XML project file
        if (create)
            mXmlFile = XmlFile::create(mFilepath, QStringLiteral("project"), 0);
        else
            mXmlFile = new XmlFile(mFilepath, mIsRestored, mIsReadOnly, QStringLiteral("project"));

        // the project seems to be ready to open, so we will create all needed objects

        mUndoStack = new UndoStack();
        mProjectLibrary = new ProjectLibrary(*this, mIsRestored, mIsReadOnly);
        mCircuit = new Circuit(*this, mIsRestored, mIsReadOnly, create);

        // Load all schematic layers
        foreach (unsigned int id, SchematicLayer::getAllLayerIDs())
            mSchematicLayers.insert(id, new SchematicLayer(id));

        // Load all schematics
        if (create)
            mSchematicsIniFile = IniFile::create(mPath.getPathTo("schematics/schematics.ini"), 0);
        else
            mSchematicsIniFile = new IniFile(mPath.getPathTo("schematics/schematics.ini"), mIsRestored, mIsReadOnly);
        QSettings* schematicsSettings = mSchematicsIniFile->createQSettings();
        int schematicsCount = schematicsSettings->beginReadArray("pages");
        for (int i = 0; i < schematicsCount; i++)
        {
            schematicsSettings->setArrayIndex(i);
            FilePath filepath = FilePath::fromRelative(mPath.getPathTo("schematics"),
                                    schematicsSettings->value("page").toString());
            Schematic* schematic = new Schematic(*this, filepath, mIsRestored, mIsReadOnly);
            addSchematic(schematic, -1, false);
        }
        schematicsSettings->endArray();
        mSchematicsIniFile->releaseQSettings(schematicsSettings);
        qDebug() << mSchematics.count() << "schematics successfully loaded!";

        mSchematicEditor = new SchematicEditor(*this, mIsReadOnly);

        if (create) save(); // write all files to harddisc
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        delete mSchematicEditor;        mSchematicEditor = 0;
        foreach (Schematic* schematic, mSchematics)
            try { removeSchematic(schematic, false, true); } catch (...) {}
        delete mSchematicsIniFile;      mSchematicsIniFile = 0;
        qDeleteAll(mSchematicLayers);   mSchematicLayers.clear();
        delete mCircuit;                mCircuit = 0;
        delete mProjectLibrary;         mProjectLibrary = 0;
        delete mUndoStack;              mUndoStack = 0;
        delete mXmlFile;                mXmlFile = 0;
        throw; // ...and rethrow the exception
    }

    // project successfully opened! :-)

    // setup the timer for automatic backups, if enabled in the settings
    int intervalSecs =  Workspace::instance().getSettings().getProjectAutosaveInterval()->getInterval();
    if ((intervalSecs > 0) && (!mIsReadOnly))
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
    Workspace::instance().unregisterOpenProject(this);

    // stop the autosave timer
    mAutoSaveTimer.stop();

    // delete all command objects in the undo stack (must be done before other important
    // objects are deleted, as undo command objects can hold pointers/references to them!)
    mUndoStack->clear();

    // free the allocated memory in the reverse order of their allocation

    delete mSchematicEditor;        mSchematicEditor = 0;

    // delete all schematics (and catch all throwed exceptions)
    foreach (Schematic* schematic, mSchematics)
        try { removeSchematic(schematic, false, true); } catch (...) {}

    delete mSchematicsIniFile;      mSchematicsIniFile = 0;
    qDeleteAll(mSchematicLayers);   mSchematicLayers.clear();
    delete mCircuit;                mCircuit = 0;
    delete mProjectLibrary;         mProjectLibrary = 0;
    delete mUndoStack;              mUndoStack = 0;
    delete mXmlFile;                mXmlFile = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

int Project::getSchematicIndex(Schematic* schematic) const noexcept
{
    return mSchematics.indexOf(schematic);
}

Schematic* Project::getSchematicByUuid(const QUuid& uuid) const noexcept
{
    foreach (Schematic* schematic, mSchematics)
    {
        if (schematic->getUuid() == uuid)
            return schematic;
    }

    return 0;
}

Schematic* Project::getSchematicByName(const QString& name) const noexcept
{
    foreach (Schematic* schematic, mSchematics)
    {
        if (schematic->getName() == name)
            return schematic;
    }

    return 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

Schematic* Project::createSchematic(const QString& name) throw (Exception)
{
    QString basename = name; /// @todo remove special characters to create a valid filename!
    FilePath filepath = mPath.getPathTo("schematics/" % basename % ".xml");
    return Schematic::create(*this, filepath, name);
}

void Project::addSchematic(Schematic* schematic, int newIndex, bool toList) throw (Exception)
{
    Q_CHECK_PTR(schematic);

    if ((newIndex < 0) || (newIndex > mSchematics.count()))
        newIndex = mSchematics.count();

    if (getSchematicByUuid(schematic->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, schematic->getUuid().toString(),
            QString(tr("There is already a schematic with the UUID \"%1\"!"))
            .arg(schematic->getUuid().toString()));
    }

    if (getSchematicByName(schematic->getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, schematic->getName(),
            QString(tr("There is already a schematic with the name \"%1\"!"))
            .arg(schematic->getName()));
    }

    mSchematics.insert(newIndex, schematic);

    if (toList)
    {
        // add schematic to "schematics/schematics.ini"
        try
        {
            updateSchematicsList(); // can throw an exception
        }
        catch (Exception& e)
        {
            mSchematics.removeAt(newIndex); // revert insert()
            throw;
        }
    }

    emit schematicAdded(newIndex);
}

void Project::removeSchematic(Schematic* schematic, bool fromList, bool deleteSchematic) throw (Exception)
{
    Q_CHECK_PTR(schematic);

    int index = mSchematics.indexOf(schematic);
    Q_ASSERT(index >= 0);
    mSchematics.removeAt(index);

    if (fromList)
    {
        // remove schematic from "schematics/schematics.ini"
        try
        {
            updateSchematicsList(); // can throw an exception
        }
        catch (Exception& e)
        {
            mSchematics.insert(index, schematic); // revert removeAt()
            throw;
        }
    }

    emit schematicRemoved(index);

    if (deleteSchematic)
        delete schematic;
}

void Project::exportSchematicsAsPdf(const FilePath& filepath) throw (Exception)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::A4);
    printer.setOrientation(QPrinter::Landscape);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setCreator(QString("EDA4U %1.%2").arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR));
    printer.setOutputFileName(filepath.toStr());

    QList<unsigned int> pages;
    for (int i = 0; i < mSchematics.count(); i++)
        pages.append(i);

    printSchematicPages(printer, pages);
}

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
    if ((!mIsRestored) && (mUndoStack->isClean())) // do not save if there are no changes
        return false;

    if (mUndoStack->isCommandActive())
    {
        // the user is executing a command at the moment, so we should not save now,
        // try it a few seconds later instead...
        QTimer::singleShot(10000, this, SLOT(autosave()));
        return false;
    }

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
    if ((!mIsRestored) && (mUndoStack->isClean()))
    {
        // no unsaved changes --> the project can be closed
        deleteLater();  // this project object will be deleted later in the event loop
        return true;
    }

    QString msg1 = tr("You have unsaved changes in the project.\n"
                      "Do you want to save them bevore closing the project?");
    QString msg2 = tr("Attention: The project was restored from a backup, so if you "
                      "don't save the project now the current state of the project (and "
                      "the backup) will be lost forever!");

    QMessageBox::StandardButton choice = QMessageBox::question(msgBoxParent,
         tr("Save Project?"), (mIsRestored ? msg1 % QStringLiteral("\n\n") % msg2 : msg1),
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

void Project::updateSchematicsList() throw (Exception)
{
    QSettings* s = mSchematicsIniFile->createQSettings(); // can throw an exception

    FilePath schematicsPath(mPath.getPathTo("schematics"));
    s->remove("pages");
    s->beginWriteArray("pages");
    for (int i = 0; i < mSchematics.count(); i++)
    {
        s->setArrayIndex(i);
        s->setValue("page", mSchematics.at(i)->getFilePath().toRelative(schematicsPath));
    }
    s->endArray();

    mSchematicsIniFile->releaseQSettings(s);
}

bool Project::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    if (mIsReadOnly)
    {
        errors.append(tr("The project was opened in read-only mode."));
        return false;
    }

    if (mUndoStack->isCommandActive())
    {
        errors.append(tr("A command is active at the moment."));
        return false;
    }

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

    // Save circuit
    if (!mCircuit->save(toOriginal, errors))
        success = false;

    // Save all schematics (*.xml files)
    foreach (Schematic* schematic, mSchematics)
    {
        if (!schematic->save(toOriginal, errors))
            success = false;
    }

    // Save "schematics/schematics.ini"
    try
    {
        mSchematicsIniFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    // if the project was restored from a backup, reset the mIsRestored flag as the current
    // state of the project is no longer a restored backup but a properly saved project
    if (mIsRestored && success && toOriginal)
        mIsRestored = false;

    return success;
}

void Project::printSchematicPages(QPrinter& printer, QList<unsigned int>& pages) throw (Exception)
{
    if (pages.isEmpty())
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("No schematic pages selected."));

    QPainter painter(&printer);

    for (int i = 0; i < pages.count(); i++)
    {
        Schematic* schematic = getSchematicByIndex(pages[i]);
        if (!schematic)
        {
            throw RuntimeError(__FILE__, __LINE__, QString(),
                QString(tr("No schematic page with the index %1 found.")).arg(pages[i]));
        }
        schematic->clearSelection();
        schematic->render(&painter, QRectF(), schematic->itemsBoundingRect(), Qt::KeepAspectRatio);

        if (i != pages.count() - 1)
        {
            if (!printer.newPage())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    tr("Unknown error while printing."));
            }
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
