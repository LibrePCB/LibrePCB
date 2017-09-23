/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/fileio/smarttextfile.h>
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/smartversionfile.h>
#include <librepcb/common/fileio/domdocument.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/systeminfo.h>
#include "project.h"
#include "library/projectlibrary.h"
#include "circuit/circuit.h"
#include "schematics/schematic.h"
#include "erc/ercmsglist.h"
#include "settings/projectsettings.h"
#include "boards/board.h"
#include <librepcb/common/application.h>
#include "schematics/schematiclayerprovider.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Project::Project(const FilePath& filepath, bool create, bool readOnly) :
    QObject(nullptr), AttributeProvider(), mPath(filepath.getParentDir()),
    mFilepath(filepath), mLock(filepath.getParentDir()), mIsRestored(false),
    mIsReadOnly(readOnly)
{
    qDebug() << (create ? "create project:" : "open project:") << filepath.toNative();

    // Check if the file extension is correct
    if (mFilepath.getSuffix() != "lpp") {
        qDebug() << mFilepath.toStr();
        throw RuntimeError(__FILE__, __LINE__,
            tr("The suffix of the project file must be \"lpp\"!"));
    }

    // Check if the filepath is valid
    if (create) {
        if (mPath.isExistingDir() && (!mPath.isEmptyDir())) {
            throw RuntimeError(__FILE__, __LINE__, QString(tr(
                "The directory \"%1\" is not empty!")).arg(mFilepath.toNative()));
        }
        FileUtils::makePath(mPath); // can throw
    } else {
        // check if the project does exist
        if (!isValidProjectDirectory(mPath)) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The directory \"%1\" does not contain a LibrePCB project."))
                .arg(mPath.toNative()));
        }
        if (!mFilepath.isExistingFile()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The file \"%1\" does not exist.")).arg(mFilepath.toNative()));
        }
        // check the project's file format version
        Version version = getProjectFileFormatVersion(mPath);
        if ((!version.isValid()) || (version != qApp->getFileFormatVersion())) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("This project was created with a newer application version.\n"
                           "You need at least LibrePCB %1 to open it.\n\n%2"))
                .arg(version.toPrettyStr(3)).arg(mFilepath.toNative()));
        }
    }

    // Check if the project is locked (already open or application was crashed). In case
    // of a crash, the user can decide if the last backup should be restored. If the
    // project should be opened, the lock file will be created/updated here.
    switch (mLock.getStatus()) // can throw
    {
        case DirectoryLock::LockStatus::Unlocked: {
            // nothing to do here (the project will be locked later)
            break;
        }
        case DirectoryLock::LockStatus::Locked: {
            if (!mIsReadOnly) {
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
            }
            break;
        }
        case DirectoryLock::LockStatus::StaleLock: {
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
        default: Q_ASSERT(false); throw LogicError(__FILE__, __LINE__);
    }

    // the project can be opened by this application, so we will lock the whole project
    if (!mIsReadOnly) {
        mLock.lock(); // can throw
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
        // try to create/open the version file
        FilePath versionFilePath = mPath.getPathTo(".librepcb-project");
        if (create) {
            mVersionFile.reset(SmartVersionFile::create(versionFilePath,  qApp->getFileFormatVersion()));
        } else {
            mVersionFile.reset(new SmartVersionFile(versionFilePath, mIsRestored, mIsReadOnly));
            // the version check was already done above, so we can use an assert here
            Q_ASSERT(mVersionFile->getVersion() <= qApp->getFileFormatVersion());
        }

        // try to create/open the XML project file
        std::unique_ptr<DomDocument> doc;
        DomElement* root = nullptr;
        if (create) {
            mXmlFile.reset(SmartXmlFile::create(mFilepath));
        } else {
            mXmlFile.reset(new SmartXmlFile(mFilepath, mIsRestored, mIsReadOnly));
            doc = mXmlFile->parseFileAndBuildDomTree();
            root = &doc->getRoot();
        }

        // load project attributes
        mLastModified = QDateTime::currentDateTime();
        if (create) {
            mName = mFilepath.getCompleteBasename();
            mAuthor = SystemInfo::getFullUsername();
            mVersion = "v1";
            mCreated = QDateTime::currentDateTime();
            mAttributes.reset(new AttributeList());
        } else {
            mName = root->getFirstChild("name", true)->getText<QString>(false);
            mAuthor = root->getFirstChild("author", true)->getText<QString>(false);
            mVersion = root->getFirstChild("version", true)->getText<QString>(false);
            mCreated = root->getFirstChild("created", true)->getText<QDateTime>(true);
            mAttributes.reset(new AttributeList(*root)); // can throw
        }

        // Create all needed objects
        mProjectSettings.reset(new ProjectSettings(*this, mIsRestored, mIsReadOnly, create));
        mProjectLibrary.reset(new ProjectLibrary(*this, mIsRestored, mIsReadOnly));
        mErcMsgList.reset(new ErcMsgList(*this, mIsRestored, mIsReadOnly, create));
        mCircuit.reset(new Circuit(*this, mIsRestored, mIsReadOnly, create));

        // Load all schematic layers
        mSchematicLayerProvider.reset(new SchematicLayerProvider(*this));

        if (!create) {
            // Load all schematics
            foreach (const DomElement* node, root->getChilds("schematic")) {
                FilePath fp = FilePath::fromRelative(mPath.getPathTo("schematics"), node->getText<QString>(true));
                Schematic* schematic = new Schematic(*this, fp, mIsRestored, mIsReadOnly);
                addSchematic(*schematic);
            }
            qDebug() << mSchematics.count() << "schematics successfully loaded!";

            // Load all boards
            foreach (const DomElement* node, root->getChilds("board")) {
                FilePath fp = FilePath::fromRelative(mPath.getPathTo("boards"), node->getText<QString>(true));
                Board* board = new Board(*this, fp, mIsRestored, mIsReadOnly);
                addBoard(*board);
            }
            qDebug() << mBoards.count() << "boards successfully loaded!";
        }

        // at this point, the whole circuit with all schematics and boards is successfully
        // loaded, so the ERC list now contains all the correct ERC messages.
        // So we can now restore the ignore state of each ERC message from the XML file.
        mErcMsgList->restoreIgnoreState(); // can throw

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

        if (create) save(true); // write all files to harddisc
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        foreach (Board* board, mBoards) {
            try { removeBoard(*board, true); } catch (...) {}
        }
        foreach (Schematic* schematic, mSchematics) {
            try { removeSchematic(*schematic, true); } catch (...) {}
        }
        throw; // ...and rethrow the exception
    }

    // project successfully opened! :-)
    qDebug() << "project successfully loaded!";
}

Project::~Project() noexcept
{
    // free the allocated memory in the reverse order of their allocation

    // delete all boards and schematics (and catch all throwed exceptions)
    foreach (Board* board, mBoards) {
        try { removeBoard(*board, true); } catch (...) {}
    }
    qDeleteAll(mRemovedBoards); mRemovedBoards.clear();
    foreach (Schematic* schematic, mSchematics) {
        try { removeSchematic(*schematic, true); } catch (...) {}
    }
    qDeleteAll(mRemovedSchematics); mRemovedSchematics.clear();

    qDebug() << "closed project:" << mFilepath.toNative();
}

/*****************************************************************************************
 *  Setters: Attributes
 ****************************************************************************************/

void Project::setName(const QString& newName) noexcept
{
    if (newName != mName) {
        mName = newName;
        emit attributesChanged();
    }
}

void Project::setAuthor(const QString& newAuthor) noexcept
{
    if (newAuthor != mAuthor) {
        mAuthor = newAuthor;
        emit attributesChanged();
    }
}

void Project::setVersion(const QString& newVersion) noexcept
{
    if (newVersion != mVersion) {
        mVersion = newVersion;
        emit attributesChanged();
    }
}

void Project::setAttributes(const AttributeList& newAttributes) noexcept
{
    if (newAttributes != *mAttributes) {
        *mAttributes = newAttributes;
        emit attributesChanged();
    }
}

/*****************************************************************************************
 *  Schematic Methods
 ****************************************************************************************/

int Project::getSchematicIndex(const Schematic& schematic) const noexcept
{
    return mSchematics.indexOf(const_cast<Schematic*>(&schematic));
}

Schematic* Project::getSchematicByUuid(const Uuid& uuid) const noexcept
{
    foreach (Schematic* schematic, mSchematics) {
        if (schematic->getUuid() == uuid)
            return schematic;
    }
    return nullptr;
}

Schematic* Project::getSchematicByName(const QString& name) const noexcept
{
    foreach (Schematic* schematic, mSchematics) {
        if (schematic->getName() == name)
            return schematic;
    }
    return nullptr;
}

Schematic* Project::createSchematic(const QString& name)
{
    QString basename = FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (basename.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid schematic name: \"%1\"")).arg(name));
    }
    FilePath filepath = mPath.getPathTo("schematics/" % basename % ".xml");
    if (filepath.isExistingFile()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The schematic exists already: \"%1\"")).arg(filepath.toNative()));
    }
    return Schematic::create(*this, filepath, name);
}

void Project::addSchematic(Schematic& schematic, int newIndex)
{
    if ((mSchematics.contains(&schematic)) || (&schematic.getProject() != this)) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (getSchematicByUuid(schematic.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a schematic with the UUID \"%1\"!"))
            .arg(schematic.getUuid().toStr()));
    }
    if (getSchematicByName(schematic.getName())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a schematic with the name \"%1\"!"))
            .arg(schematic.getName()));
    }

    if ((newIndex < 0) || (newIndex > mSchematics.count())) {
        newIndex = mSchematics.count();
    }

    schematic.addToProject(); // can throw
    mSchematics.insert(newIndex, &schematic);

    if (mRemovedSchematics.contains(&schematic)) {
        mRemovedSchematics.removeOne(&schematic);
    }

    emit schematicAdded(newIndex);
    emit attributesChanged();
}

void Project::removeSchematic(Schematic& schematic, bool deleteSchematic)
{
    if ((!mSchematics.contains(&schematic)) || (mRemovedSchematics.contains(&schematic))) {
        throw LogicError(__FILE__, __LINE__);
    }
    if ((!deleteSchematic) && (!schematic.isEmpty())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There are still elements in the schematic \"%1\"!"))
            .arg(schematic.getName()));
    }

    int index = getSchematicIndex(schematic);
    Q_ASSERT(index >= 0);

    schematic.removeFromProject(); // can throw
    mSchematics.removeAt(index);

    emit schematicRemoved(index);
    emit attributesChanged();

    if (deleteSchematic) {
        delete &schematic;
    } else {
        mRemovedSchematics.append(&schematic);
    }
}

void Project::exportSchematicsAsPdf(const FilePath& filepath)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::A4);
    printer.setOrientation(QPrinter::Landscape);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setCreator(QString("LibrePCB %1").arg(qApp->applicationVersion()));
    printer.setOutputFileName(filepath.toStr());

    QList<int> pages;
    for (int i = 0; i < mSchematics.count(); i++)
        pages.append(i);

    printSchematicPages(printer, pages);

    QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.toStr()));
}

/*****************************************************************************************
 *  Board Methods
 ****************************************************************************************/

int Project::getBoardIndex(const Board& board) const noexcept
{
    return mBoards.indexOf(const_cast<Board*>(&board));
}

Board* Project::getBoardByUuid(const Uuid& uuid) const noexcept
{
    foreach (Board* board, mBoards) {
        if (board->getUuid() == uuid)
            return board;
    }
    return nullptr;
}

Board* Project::getBoardByName(const QString& name) const noexcept
{
    foreach (Board* board, mBoards) {
        if (board->getName() == name)
            return board;
    }
    return nullptr;
}

Board* Project::createBoard(const QString& name)
{
    QString basename = FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (basename.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid board name: \"%1\"")).arg(name));
    }
    FilePath filepath = mPath.getPathTo("boards/" % basename % ".xml");
    if (filepath.isExistingFile()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The board exists already: \"%1\"")).arg(filepath.toNative()));
    }
    return Board::create(*this, filepath, name);
}

Board* Project::createBoard(const Board& other, const QString& name)
{
    QString basename = FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (basename.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid board name: \"%1\"")).arg(name));
    }
    FilePath filepath = mPath.getPathTo("boards/" % basename % ".xml");
    if (filepath.isExistingFile()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The board exists already: \"%1\"")).arg(filepath.toNative()));
    }
    return new Board(other, filepath, name);
}

void Project::addBoard(Board& board, int newIndex)
{
    if ((mBoards.contains(&board)) || (&board.getProject() != this)) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (getBoardByUuid(board.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a board with the UUID \"%1\"!"))
            .arg(board.getUuid().toStr()));
    }
    if (getBoardByName(board.getName())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a board with the name \"%1\"!"))
            .arg(board.getName()));
    }

    if ((newIndex < 0) || (newIndex > mBoards.count())) {
        newIndex = mBoards.count();
    }

    board.addToProject(); // can throw
    mBoards.insert(newIndex, &board);

    if (mRemovedBoards.contains(&board)) {
        mRemovedBoards.removeOne(&board);
    }

    emit boardAdded(newIndex);
    emit attributesChanged();
}

void Project::removeBoard(Board& board, bool deleteBoard)
{
    if ((!mBoards.contains(&board)) || (mRemovedBoards.contains(&board))) {
        throw LogicError(__FILE__, __LINE__);
    }

    int index = getBoardIndex(board);
    Q_ASSERT(index >= 0);

    board.removeFromProject(); // can throw
    mBoards.removeAt(index);

    emit boardRemoved(index);
    emit attributesChanged();

    if (deleteBoard) {
        delete &board;
    } else {
        mRemovedBoards.append(&board);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Project::save(bool toOriginal)
{
    QStringList errors;

    if (!save(toOriginal, errors))
    {
        QString msg = QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n"));
        throw RuntimeError(__FILE__, __LINE__, msg);
    }
    Q_ASSERT(errors.isEmpty());
}

/*****************************************************************************************
 *  Inherited from AttributeProvider
 ****************************************************************************************/

QString Project::getUserDefinedAttributeValue(const QString& key) const noexcept
{
    if (std::shared_ptr<Attribute> attr = mAttributes->find(key)) {
        return attr->getValueTr(true);
    }  else {
        return QString();
    }
}

QString Project::getBuiltInAttributeValue(const QString& key) const noexcept
{
    if (key == QLatin1String("PROJECT")) {
        return mName;
    } else if (key == QLatin1String("PROJECT_DIRPATH")) {
        return mPath.toNative();
    } else if (key == QLatin1String("PROJECT_BASENAME")) {
        return mFilepath.getBasename();
    } else if (key == QLatin1String("PROJECT_FILENAME")) {
        return mFilepath.getFilename();
    } else if (key == QLatin1String("PROJECT_FILEPATH")) {
        return mFilepath.toNative();
    } else if (key == QLatin1String("CREATED_DATE")) {
        return mCreated.date().toString(Qt::ISODate);
    } else if (key == QLatin1String("CREATED_TIME")) {
        return mCreated.time().toString(Qt::ISODate);
    } else if (key == QLatin1String("MODIFIED_DATE")) {
        return mLastModified.date().toString(Qt::ISODate);
    } else if (key == QLatin1String("MODIFIED_TIME")) {
        return mLastModified.time().toString(Qt::ISODate);
    } else if (key == QLatin1String("AUTHOR")) {
        return mAuthor;
    } else if (key == QLatin1String("VERSION")) {
        return mVersion;
    } else if (key == QLatin1String("PAGES")) {
        return QString::number(mSchematics.count());
    } else if (key == QLatin1String("PAGE_X_OF_Y")) {
        return "Page #PAGE of #PAGES"; // do not translate this, must be the same for every user!
    } else {
        return QString();
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

bool Project::isValidProjectDirectory(const FilePath& dir) noexcept
{
    return dir.getPathTo(".librepcb-project").isExistingFile();
}

Version Project::getProjectFileFormatVersion(const FilePath& dir)
{
    SmartVersionFile versionFile(dir.getPathTo(".librepcb-project"), false, true);
    return versionFile.getVersion();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Project::checkAttributesValidity() const noexcept
{
    if (mName.isEmpty())    return false;
    return true;
}

void Project::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    // metadata
    root.appendTextChild("name", mName);
    root.appendTextChild("author", mAuthor);
    root.appendTextChild("version", mVersion);
    root.appendTextChild("created", mCreated);

    // attributes
    mAttributes->serialize(root);

    // schematics
    FilePath schematicsPath(mPath.getPathTo("schematics"));
    foreach (Schematic* schematic, mSchematics)
        root.appendTextChild("schematic", schematic->getFilePath().toRelative(schematicsPath));

    // boards
    FilePath boardsPath(mPath.getPathTo("boards"));
    foreach (Board* board, mBoards)
        root.appendTextChild("board", board->getFilePath().toRelative(boardsPath));
}

bool Project::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    if (mIsReadOnly)
    {
        errors.append(tr("The project was opened in read-only mode."));
        return false;
    }

    // Save version file
    try
    {
        mVersionFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
    }

    // Save *.lpp project file
    try
    {
        DomDocument doc(*serializeToDomElement("project"));
        mXmlFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
    }

    // Save circuit
    if (!mCircuit->save(toOriginal, errors))
        success = false;

    // Save all removed schematics (*.xml files)
    foreach (Schematic* schematic, mRemovedSchematics)
    {
        if (!schematic->save(toOriginal, errors))
            success = false;
    }
    // Save all added schematics (*.xml files)
    foreach (Schematic* schematic, mSchematics)
    {
        if (!schematic->save(toOriginal, errors))
            success = false;
    }

    // Save all removed boards (*.xml files)
    foreach (Board* board, mRemovedBoards)
    {
        if (!board->save(toOriginal, errors))
            success = false;
    }
    // Save all added boards (*.xml files)
    foreach (Board* board, mBoards)
    {
        if (!board->save(toOriginal, errors))
            success = false;
    }

    // Save library
    if (!mProjectLibrary->save(toOriginal, errors))
        success = false;

    // Save settings
    if (!mProjectSettings->save(toOriginal, errors))
        success = false;

    // Save ERC messages list
    if (!mErcMsgList->save(toOriginal, errors))
        success = false;

    // if the project was restored from a backup, reset the mIsRestored flag as the current
    // state of the project is no longer a restored backup but a properly saved project
    if (mIsRestored && success && toOriginal)
        mIsRestored = false;

    // update the "last modified datetime" attribute of the project
    mLastModified = QDateTime::currentDateTime();
    emit attributesChanged();

    return success;
}

void Project::printSchematicPages(QPrinter& printer, QList<int>& pages)
{
    if (pages.isEmpty())
        throw RuntimeError(__FILE__, __LINE__, tr("No schematic pages selected."));

    QPainter painter(&printer);

    for (int i = 0; i < pages.count(); i++)
    {
        Schematic* schematic = getSchematicByIndex(pages[i]);
        if (!schematic)
        {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("No schematic page with the index %1 found.")).arg(pages[i]));
        }
        schematic->clearSelection();
        schematic->renderToQPainter(painter);

        if (i != pages.count() - 1)
        {
            if (!printer.newPage())
            {
                throw RuntimeError(__FILE__, __LINE__,
                    tr("Unknown error while printing."));
            }
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
