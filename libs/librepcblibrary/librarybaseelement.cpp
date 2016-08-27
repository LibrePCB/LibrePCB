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
#include <librepcbcommon/fileio/smartversionfile.h>
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/fileio/fileutils.h>
#include <librepcbcommon/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryBaseElement::LibraryBaseElement(bool dirnameMustBeUuid, const QString& shortElementName,
                                       const QString& xmlRootNodeName, const Uuid& uuid,
                                       const Version& version, const QString& author,
                                       const QString& name_en_US,
                                       const QString& description_en_US,
                                       const QString& keywords_en_US) throw (Exception) :
    QObject(nullptr), mDirectory(FilePath::getRandomTempPath()),
    mDirectoryIsTemporary(true), mOpenedReadOnly(false),
    mDirectoryBasenameMustBeUuid(dirnameMustBeUuid),
    mShortElementName(shortElementName), mXmlRootNodeName(xmlRootNodeName),
    mUuid(uuid), mVersion(version), mAuthor(author),
    mCreated(QDateTime::currentDateTime()), mLastModified(QDateTime::currentDateTime())
{
    if (!mDirectory.mkPath()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Could not create temporary directory \"%1\"."))
            .arg(mDirectory.toNative()));
    }

    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
    mKeywords.insert("en_US", keywords_en_US);
}

LibraryBaseElement::LibraryBaseElement(const FilePath& elementDirectory,
                                       bool dirnameMustBeUuid,
                                       const QString& shortElementName,
                                       const QString& xmlRootNodeName, bool readOnly) throw (Exception) :
    QObject(nullptr), mDirectory(elementDirectory),mDirectoryIsTemporary(false),
    mOpenedReadOnly(readOnly), mDirectoryBasenameMustBeUuid(dirnameMustBeUuid),
    mShortElementName(shortElementName), mXmlRootNodeName(xmlRootNodeName)
{
    // check if the directory is a library element
    if (!isDirectoryLibraryElement(mDirectory)) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Directory is not a LibrePCB library element: \"%1\""))
            .arg(mDirectory.toNative()));
    }

    // check directory suffix
    if (mDirectory.getSuffix() != mShortElementName) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Directory name does not end with \".%1\" : \"%2\""))
            .arg(mShortElementName, mDirectory.toNative()));
    }

    // check directory basename
    Uuid dirUuid(mDirectory.getBasename());
    if (mDirectoryBasenameMustBeUuid && dirUuid.isNull()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Directory name is not a valid UUID: \"%1\""))
            .arg(mDirectory.toNative()));
    }

    // read version number from version file
    mLoadingElementFileVersion = readFileVersionOfElementDirectory(mDirectory);
    if (!(mLoadingElementFileVersion <= qApp->getAppVersion())) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The library element %1 was created with a newer application "
                       "version. You need at least LibrePCB version %2 to open it."))
            .arg(mDirectory.toNative()).arg(mLoadingElementFileVersion.toPrettyStr(3)));
    }

    // open main XML file
    FilePath xmlFilePath = mDirectory.getPathTo(mShortElementName % ".xml");
    SmartXmlFile xmlFile(xmlFilePath, false, true);
    mLoadingXmlFileDocument = xmlFile.parseFileAndBuildDomTree();
    const XmlDomElement& root = mLoadingXmlFileDocument->getRoot(mXmlRootNodeName); // checks the name
    const XmlDomElement& metaElement = *root.getFirstChild("meta", true);

    // read attributes
    mUuid = metaElement.getFirstChild("uuid", true)->getText<Uuid>(true);
    mVersion = metaElement.getFirstChild("version", true)->getText<Version>(true);
    mAuthor = metaElement.getFirstChild("author", true)->getText<QString>(true);
    mCreated = metaElement.getFirstChild("created", true)->getText<QDateTime>(true);
    mLastModified = metaElement.getFirstChild("last_modified", true)->getText<QDateTime>(true);

    // read names, descriptions and keywords in all available languages
    readLocaleDomNodes(metaElement, "name", mNames, true);
    readLocaleDomNodes(metaElement, "description", mDescriptions, false);
    readLocaleDomNodes(metaElement, "keywords", mKeywords, false);

    // check if the UUID equals to the directory basename
    if (mDirectoryBasenameMustBeUuid && (mUuid != dirUuid)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1/%2").arg(mUuid.toStr(), dirUuid.toStr()),
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

QString LibraryBaseElement::checkDirectoryNameValidity(const FilePath& dir) const noexcept
{
    if (mDirectoryBasenameMustBeUuid && (dir.getCompleteBasename() != mUuid.toStr())) {
        return QString(tr("The directory basename must be equal to the element's UUID."));
    } else if (dir.getSuffix() != mShortElementName) {
        return QString(tr("The directory name suffix must be \".%1\".")).arg(mShortElementName);
    } else {
        return QString();
    }
}

QString LibraryBaseElement::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString LibraryBaseElement::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

QString LibraryBaseElement::getKeywords(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mKeywords, localeOrder);
}

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

void LibraryBaseElement::save() throw (Exception)
{
    if (mOpenedReadOnly) {
        throw RuntimeError(__FILE__, __LINE__, mDirectory.toStr(),
            QString(tr("Library element was opened in read-only mode: \"%1\""))
            .arg(mDirectory.toNative()));
    }

    // save xml file
    FilePath xmlFilePath = mDirectory.getPathTo(mShortElementName % ".xml");
    QScopedPointer<XmlDomDocument> doc(new XmlDomDocument(*serializeToXmlDomElement()));
    QScopedPointer<SmartXmlFile> xmlFile(SmartXmlFile::create(xmlFilePath));
    xmlFile->save(*doc, true);

    // save version number file
    QScopedPointer<SmartVersionFile> versionFile(SmartVersionFile::create(
        mDirectory.getPathTo(".version"), qApp->getFileFormatVersion()));
    versionFile->save(true);
}

void LibraryBaseElement::saveTo(const FilePath& destination) throw (Exception)
{
    // copy to new directory and remove source directory if it was temporary
    copyTo(destination, mDirectoryIsTemporary);
}

void LibraryBaseElement::saveIntoParentDirectory(const FilePath& parentDir) throw (Exception)
{
    FilePath elemDir = parentDir.getPathTo(mUuid.toStr() % "." % mShortElementName);
    saveTo(elemDir);
}

void LibraryBaseElement::moveTo(const FilePath& destination) throw (Exception)
{
    // copy to new directory and remove source directory
    copyTo(destination, true);
}

void LibraryBaseElement::moveIntoParentDirectory(const FilePath& parentDir) throw (Exception)
{
    FilePath elemDir = parentDir.getPathTo(mUuid.toStr() % "." % mShortElementName);
    moveTo(elemDir);
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

void LibraryBaseElement::cleanupAfterLoadingElementFromFile() noexcept
{
    mLoadingElementFileVersion = Version();
    mLoadingXmlFileDocument.reset(); // destroy the whole XML DOM tree
}

void LibraryBaseElement::copyTo(const FilePath& destination, bool removeSource) throw (Exception)
{
    if (destination != mDirectory) {
        // check destination directory name validity
        QString errMsg = checkDirectoryNameValidity(destination);
        if (!errMsg.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__, QString(),
                 QString(tr("Invalid library element directory name \"%1\": %2"))
                .arg(destination.getFilename(), errMsg));
        }

        // check if destination directory exists already
        if (destination.isExistingDir() || destination.isExistingFile()) {
            throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr("Could not copy "
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

XmlDomElement* LibraryBaseElement::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) {
        throw LogicError(__FILE__, __LINE__, QString(),
            tr("The library element cannot be saved because it is not valid."));
    }

    QScopedPointer<XmlDomElement> root(new XmlDomElement(mXmlRootNodeName));
    XmlDomElement* meta = root->appendChild("meta");
    meta->appendTextChild("uuid", mUuid);
    meta->appendTextChild("version", mVersion);
    meta->appendTextChild("author", mAuthor);
    meta->appendTextChild("created", mCreated);
    meta->appendTextChild("last_modified", mLastModified);
    foreach (const QString& locale, mNames.keys())
        meta->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        meta->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mKeywords.keys())
        meta->appendTextChild("keywords", mKeywords.value(locale))->setAttribute("locale", locale);
    return root.take();
}

bool LibraryBaseElement::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (!mVersion.isValid())                return false;
    if (mNames.value("en_US").isEmpty())    return false;
    if (!mDescriptions.contains("en_US"))   return false;
    if (!mKeywords.contains("en_US"))       return false;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

void LibraryBaseElement::readLocaleDomNodes(const XmlDomElement& parentNode,
                                            const QString& childNodesName,
                                            QMap<QString, QString>& list,
                                            bool throwIfValueEmpty) throw (Exception)
{
    for (XmlDomElement* node = parentNode.getFirstChild(childNodesName, false); node;
         node = node->getNextSibling(childNodesName))
    {
        QString locale = node->getAttribute<QString>("locale", true);
        Q_ASSERT(!locale.isEmpty());
        if (list.contains(locale)) {
            throw RuntimeError(__FILE__, __LINE__, parentNode.getDocFilePath().toStr(),
                QString(tr("Locale \"%1\" defined multiple times in \"%2\"."))
                .arg(locale, parentNode.getDocFilePath().toNative()));
        }
        list.insert(locale, node->getText<QString>(throwIfValueEmpty));
    }

    if (!list.contains("en_US")) {
        throw RuntimeError(__FILE__, __LINE__, parentNode.getDocFilePath().toStr(), QString(
            tr("At least one entry in \"%1\" has no translation for locale \"en_US\"."))
            .arg(parentNode.getDocFilePath().toNative()));
    }
}

QString LibraryBaseElement::localeStringFromList(const QMap<QString, QString>& list,
                                                 const QStringList& localeOrder,
                                                 QString* usedLocale) throw (Exception)
{
    // search in the specified locale order
    foreach (const QString& locale, localeOrder) {
        if (list.contains(locale)) {
            if (usedLocale) *usedLocale = locale;
            return list.value(locale);
        }
    }

    // try the fallback locale "en_US"
    if (list.contains("en_US")) {
        if (usedLocale) *usedLocale = "en_US";
        return list.value("en_US");
    }

    throw RuntimeError(__FILE__, __LINE__, QString(), tr("No translation found."));
}

bool LibraryBaseElement::isDirectoryLibraryElement(const FilePath& dir) noexcept
{
    // The presence of a ".version" file in the element's directory is enought
    // to say that the directory represents a LibrePCB library element ;)
    FilePath versionFilePath = dir.getPathTo(".version");
    return versionFilePath.isExistingFile();
}

Version LibraryBaseElement::readFileVersionOfElementDirectory(const FilePath& dir) throw (Exception)
{
    FilePath versionFilePath = dir.getPathTo(".version");
    SmartVersionFile versionFile(versionFilePath, false, true);
    return versionFile.getVersion();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
