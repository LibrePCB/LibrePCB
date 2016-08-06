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

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryBaseElement::LibraryBaseElement(const QString& xmlFileNamePrefix,
                                       const QString& xmlRootNodeName, const Uuid& uuid,
                                       const Version& version, const QString& author,
                                       const QString& name_en_US,
                                       const QString& description_en_US,
                                       const QString& keywords_en_US) throw (Exception) :
    QObject(nullptr), mDirectory(FilePath::getRandomTempPath()),
    mDirectoryIsTemporary(true), mXmlFileNamePrefix(xmlFileNamePrefix),
    mXmlRootNodeName(xmlRootNodeName), mDomTreeParsed(false), mOpenedReadOnly(false),
    mUuid(uuid), mVersion(version), mAuthor(author),
    mCreated(QDateTime::currentDateTime()), mLastModified(QDateTime::currentDateTime())
{
    if (!mDirectory.mkPath()) {
        qWarning() << "Could not create temporary directory:" << mDirectory.toNative();
    }

    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
    mKeywords.insert("en_US", keywords_en_US);
}

LibraryBaseElement::LibraryBaseElement(const FilePath& elementDirectory,
                                       const QString& xmlFileNamePrefix,
                                       const QString& xmlRootNodeName, bool readOnly) throw (Exception) :
    QObject(nullptr), mDirectory(elementDirectory), mDirectoryIsTemporary(false),
    mXmlFileNamePrefix(xmlFileNamePrefix), mXmlRootNodeName(xmlRootNodeName),
    mDomTreeParsed(false), mOpenedReadOnly(readOnly)
{
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
    XmlDomDocument doc(*serializeToXmlDomElement());
    QScopedPointer<SmartXmlFile> xmlFile(SmartXmlFile::create(mDirectory.getPathTo(mXmlFileNamePrefix % ".xml")));
    xmlFile->save(doc, true);

    // save version number file
    QScopedPointer<SmartVersionFile> versionFile(SmartVersionFile::create(
        mDirectory.getPathTo(".version"), Version(QString::number(APP_VERSION_MAJOR))));
    versionFile->save(true);
}

void LibraryBaseElement::saveTo(const FilePath& parentDir) throw (Exception)
{
    if (parentDir != mDirectory.getParentDir()) {
        QString dirname = QString("%1.%2").arg(mUuid.toStr()).arg(mXmlFileNamePrefix);
        FilePath destinationDir = parentDir.getPathTo(dirname);

        // remove destination directory
        if (!QDir(destinationDir.toStr()).removeRecursively()) {
            throw RuntimeError(__FILE__, __LINE__, destinationDir.toStr(),
                QString(tr("Could not remove the directory \"%1\"."))
                .arg(destinationDir.toNative()));
        }

        // TODO: copy current directory to destination

        // if current directory is temporary, try to remove it
        if (mDirectoryIsTemporary) {
            if (!QDir(mDirectory.toStr()).removeRecursively()) {
                qWarning() << "Could not remove temporary directory:" << mDirectory.toNative();
            }
        }

        mDirectory = destinationDir;
        mDirectoryIsTemporary = false;
        mOpenedReadOnly = false;
    }

    save();
}

void LibraryBaseElement::moveTo(const FilePath& parentDir) throw (Exception)
{
    if (parentDir != mDirectory.getParentDir()) {
        QString dirname = QString("%1.%2").arg(mUuid.toStr()).arg(mXmlFileNamePrefix);
        FilePath destinationDir = parentDir.getPathTo(dirname);

        // remove destination directory
        if (!QDir(destinationDir.toStr()).removeRecursively()) {
            throw RuntimeError(__FILE__, __LINE__, destinationDir.toStr(),
                QString(tr("Could not remove the directory \"%1\"."))
                .arg(destinationDir.toNative()));
        }

        // TODO: copy current directory to destination

        // remove current directory
        if (!QDir(mDirectory.toStr()).removeRecursively()) {
            throw RuntimeError(__FILE__, __LINE__, mDirectory.toStr(),
                QString(tr("Could not remove the directory \"%1\"."))
                .arg(mDirectory.toNative()));
        }

        mDirectory = destinationDir;
        mDirectoryIsTemporary = false;
        mOpenedReadOnly = false;
    }

    save();
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

void LibraryBaseElement::readFromFile() throw (Exception)
{
    Q_ASSERT(mDomTreeParsed == false);

    // check directory
    Uuid dirUuid = Uuid(mDirectory.getBasename());
    if ((!mDirectory.isExistingDir()) || (dirUuid.isNull()))
    {
        throw RuntimeError(__FILE__, __LINE__, dirUuid.toStr(),
            QString(tr("Directory does not exist or is not a valid UUID: \"%1\""))
            .arg(mDirectory.toNative()));
    }

    // read version number from version file
    FilePath versionFilePath = mDirectory.getPathTo(".version");
    QScopedPointer<SmartVersionFile> versionFile(
        new SmartVersionFile(versionFilePath, false, true));
    if (!(versionFile->getVersion() <= Version(qApp->applicationVersion()))) {
        throw RuntimeError(__FILE__, __LINE__, QString::number(APP_VERSION_MAJOR),
            QString(tr("The library element %1 was created with a newer application "
                       "version. You need at least version %2 to open this file."))
            .arg(mDirectory.toNative()).arg(versionFile->getVersion().toStr()));
    }

    // open XML file
    FilePath xmlFilePath = mDirectory.getPathTo(mXmlFileNamePrefix % ".xml");
    SmartXmlFile xmlFile(xmlFilePath, false, true);
    QSharedPointer<XmlDomDocument> doc = xmlFile.parseFileAndBuildDomTree();
    parseDomTree(doc->getRoot());

    // check UUID
    if (mUuid != dirUuid)
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1/%2").arg(mUuid.toStr(), dirUuid.toStr()),
            QString(tr("UUID mismatch between element directory and XML file: \"%1\""))
            .arg(xmlFilePath.toNative()));
    }

    // check attributes
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    Q_ASSERT(mDomTreeParsed == true);
}

void LibraryBaseElement::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    Q_ASSERT(mDomTreeParsed == false);

    // read attributes
    mUuid = root.getFirstChild("meta/uuid", true, true)->getText<Uuid>(true);
    mVersion = root.getFirstChild("meta/version", true, true)->getText<Version>(true);
    mAuthor = root.getFirstChild("meta/author", true, true)->getText<QString>(true);
    mCreated = root.getFirstChild("meta/created", true, true)->getText<QDateTime>(true);
    mLastModified = root.getFirstChild("meta/last_modified", true, true)->getText<QDateTime>(true);

    // read names, descriptions and keywords in all available languages
    readLocaleDomNodes(*root.getFirstChild("meta", true), "name", mNames);
    readLocaleDomNodes(*root.getFirstChild("meta", true), "description", mDescriptions);
    readLocaleDomNodes(*root.getFirstChild("meta", true), "keywords", mKeywords);

    mDomTreeParsed = true;
}

XmlDomElement* LibraryBaseElement::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

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
                                            QMap<QString, QString>& list) throw (Exception)
{
    for (XmlDomElement* node = parentNode.getFirstChild(childNodesName, false); node;
         node = node->getNextSibling(childNodesName))
    {
        QString locale = node->getAttribute<QString>("locale", true);
        if (locale.isEmpty())
        {
            throw RuntimeError(__FILE__, __LINE__, parentNode.getDocFilePath().toStr(),
                QString(tr("Entry without locale found in \"%1\"."))
                .arg(parentNode.getDocFilePath().toNative()));
        }
        if (list.contains(locale))
        {
            throw RuntimeError(__FILE__, __LINE__, parentNode.getDocFilePath().toStr(),
                QString(tr("Locale \"%1\" defined multiple times in \"%2\"."))
                .arg(locale, parentNode.getDocFilePath().toNative()));
        }
        list.insert(locale, node->getText<QString>(false));
    }

    if (!list.contains("en_US"))
    {
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
    foreach (const QString& locale, localeOrder)
    {
        if (list.contains(locale))
        {
            if (usedLocale) *usedLocale = locale;
            return list.value(locale);
        }
    }

    // try the fallback locale "en_US"
    if (list.contains("en_US"))
    {
        if (usedLocale) *usedLocale = "en_US";
        return list.value("en_US");
    }

    throw RuntimeError(__FILE__, __LINE__, QString(), tr("No translation found."));
}

bool LibraryBaseElement::isDirectoryValidElement(const FilePath& dir) noexcept
{
    // TODO: check version number
    // find the xml file with the highest file version number
    QString filename = QString("%1.xml").arg(dir.getSuffix());
    return dir.getPathTo(filename).isExistingFile();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
