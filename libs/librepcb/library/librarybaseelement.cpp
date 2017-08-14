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
#include "librarybaseelement.h"
#include <librepcb/common/fileio/smartversionfile.h>
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/domdocument.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryBaseElement::LibraryBaseElement(bool dirnameMustBeUuid, const QString& shortElementName,
                                       const QString& longElementName, const Uuid& uuid,
                                       const Version& version, const QString& author,
                                       const QString& name_en_US,
                                       const QString& description_en_US,
                                       const QString& keywords_en_US) :
    QObject(nullptr), mDirectory(FilePath::getRandomTempPath()),
    mDirectoryIsTemporary(true), mOpenedReadOnly(false),
    mDirectoryNameMustBeUuid(dirnameMustBeUuid),
    mShortElementName(shortElementName), mLongElementName(longElementName),
    mUuid(uuid), mVersion(version), mAuthor(author),
    mCreated(QDateTime::currentDateTime()), mLastModified(QDateTime::currentDateTime()),
    mIsDeprecated(false)
{
    FileUtils::makePath(mDirectory); // can throw

    mNames.setDefaultValue(name_en_US);
    mDescriptions.setDefaultValue(description_en_US);
    mKeywords.setDefaultValue(keywords_en_US);
}

LibraryBaseElement::LibraryBaseElement(const FilePath& elementDirectory,
                                       bool dirnameMustBeUuid,
                                       const QString& shortElementName,
                                       const QString& longElementName, bool readOnly) :
    QObject(nullptr), mDirectory(elementDirectory),mDirectoryIsTemporary(false),
    mOpenedReadOnly(readOnly), mDirectoryNameMustBeUuid(dirnameMustBeUuid),
    mShortElementName(shortElementName), mLongElementName(longElementName)
{
    // determine the filepath to the version file
    FilePath versionFilePath = mDirectory.getPathTo(".librepcb-" % mShortElementName);

    // check if the directory is a library element
    if (!versionFilePath.isExistingFile()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Directory is not a library element of type %1: \"%2\""))
            .arg(mLongElementName, mDirectory.toNative()));
    }

    // check directory name
    Uuid dirUuid(mDirectory.getFilename());
    if (mDirectoryNameMustBeUuid && dirUuid.isNull()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Directory name is not a valid UUID: \"%1\""))
            .arg(mDirectory.toNative()));
    }

    // read version number from version file
    SmartVersionFile versionFile(versionFilePath, false, true);
    mLoadingElementFileVersion = versionFile.getVersion();
    if (mLoadingElementFileVersion != qApp->getAppVersion()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The library element %1 was created with a newer application "
                       "version. You need at least LibrePCB version %2 to open it."))
            .arg(mDirectory.toNative()).arg(mLoadingElementFileVersion.toPrettyStr(3)));
    }

    // open main XML file
    FilePath xmlFilePath = mDirectory.getPathTo(mLongElementName % ".xml");
    SmartXmlFile xmlFile(xmlFilePath, false, true);
    mLoadingXmlFileDocument = xmlFile.parseFileAndBuildDomTree();
    const DomElement& root = mLoadingXmlFileDocument->getRoot(mLongElementName);

    // read attributes
    mUuid = root.getFirstChild("uuid", true)->getText<Uuid>(true);
    mVersion = root.getFirstChild("version", true)->getText<Version>(true);
    mAuthor = root.getFirstChild("author", true)->getText<QString>(true);
    mCreated = root.getFirstChild("created", true)->getText<QDateTime>(true);
    mLastModified = root.getFirstChild("last_modified", true)->getText<QDateTime>(true);
    mIsDeprecated = root.getFirstChild("deprecated", true)->getText<bool>(true);

    // read names, descriptions and keywords in all available languages
    mNames.loadFromDomElement(root);
    mDescriptions.loadFromDomElement(root);
    mKeywords.loadFromDomElement(root);

    // check if the UUID equals to the directory basename
    if (mDirectoryNameMustBeUuid && (mUuid != dirUuid)) {
        qDebug() << mUuid << "!=" << dirUuid;
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("UUID mismatch between element directory and XML file: \"%1\""))
            .arg(xmlFilePath.toNative()));
    }
}

LibraryBaseElement::~LibraryBaseElement() noexcept
{
    if (mDirectoryIsTemporary) {
        if (!QDir(mDirectory.toStr()).removeRecursively()) {
            qWarning() << "Could not remove temporary directory:" << mDirectory.toNative();
        }
    }
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QStringList LibraryBaseElement::getAllAvailableLocales() const noexcept
{
    QStringList list;
    list.append(mNames.keys());
    list.append(mDescriptions.keys());
    list.append(mKeywords.keys());
    list.removeDuplicates();
    list.sort(Qt::CaseSensitive);
    return list;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void LibraryBaseElement::save()
{
    if (mOpenedReadOnly) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Library element was opened in read-only mode: \"%1\""))
            .arg(mDirectory.toNative()));
    }

    // save xml file
    FilePath xmlFilePath = mDirectory.getPathTo(mLongElementName % ".xml");
    QScopedPointer<DomElement> root(serializeToDomElement(mLongElementName));
    QScopedPointer<DomDocument> doc(new DomDocument(*root.take()));
    QScopedPointer<SmartXmlFile> xmlFile(SmartXmlFile::create(xmlFilePath));
    xmlFile->save(*doc, true);

    // save version number file
    QScopedPointer<SmartVersionFile> versionFile(SmartVersionFile::create(
        mDirectory.getPathTo(".librepcb-" % mShortElementName),
        qApp->getFileFormatVersion()));
    versionFile->save(true);
}

void LibraryBaseElement::saveTo(const FilePath& destination)
{
    // copy to new directory and remove source directory if it was temporary
    copyTo(destination, mDirectoryIsTemporary);
}

void LibraryBaseElement::saveIntoParentDirectory(const FilePath& parentDir)
{
    FilePath elemDir = parentDir.getPathTo(mUuid.toStr());
    saveTo(elemDir);
}

void LibraryBaseElement::moveTo(const FilePath& destination)
{
    // copy to new directory and remove source directory
    copyTo(destination, true);
}

void LibraryBaseElement::moveIntoParentDirectory(const FilePath& parentDir)
{
    FilePath elemDir = parentDir.getPathTo(mUuid.toStr());
    moveTo(elemDir);
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

void LibraryBaseElement::cleanupAfterLoadingElementFromFile() noexcept
{
    mLoadingXmlFileDocument.reset(); // destroy the whole XML DOM tree
}

void LibraryBaseElement::copyTo(const FilePath& destination, bool removeSource)
{
    if (destination != mDirectory) {
        // check destination directory name validity
        if (mDirectoryNameMustBeUuid && (destination.getFilename() != mUuid.toStr())) {
            throw RuntimeError(__FILE__, __LINE__,
                 QString(tr("Library element directory name is not a valid UUID: \"%1\""))
                .arg(destination.getFilename()));
        }

        // check if destination directory exists already
        if (destination.isExistingDir() || destination.isExistingFile()) {
            throw RuntimeError(__FILE__, __LINE__, QString(tr("Could not copy "
                "library element \"%1\" to \"%2\" because the directory exists already."))
                .arg(mDirectory.toNative(), destination.toNative()));
        }

        // copy current directory to destination
        FileUtils::copyDirRecursively(mDirectory, destination);

        // memorize the current directory
        FilePath sourceDir = mDirectory;

        // save the library element to the destination directory
        mDirectory = destination;
        mDirectoryIsTemporary = false;
        mOpenedReadOnly = false;
        save();

        // remove source directory if required
        if (removeSource) {
            FileUtils::removeDirRecursively(sourceDir);
        }
    } else {
        // no copy action required, just save the element
        save();
    }
}

void LibraryBaseElement::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) {
        throw LogicError(__FILE__, __LINE__,
            tr("The library element cannot be saved because it is not valid."));
    }

    root.appendTextChild("uuid", mUuid);
    root.appendTextChild("version", mVersion);
    root.appendTextChild("author", mAuthor);
    root.appendTextChild("created", mCreated);
    root.appendTextChild("last_modified", mLastModified);
    root.appendTextChild("deprecated", mIsDeprecated);
    mNames.serialize(root);
    mDescriptions.serialize(root);
    mKeywords.serialize(root);
}

bool LibraryBaseElement::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (!mVersion.isValid())                return false;
    if (mNames.getDefaultValue().isEmpty()) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
