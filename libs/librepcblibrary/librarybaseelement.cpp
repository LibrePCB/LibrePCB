/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryBaseElement::LibraryBaseElement(const QString& xmlFileNamePrefix,
                                       const QString& xmlRootNodeName, const QUuid& uuid,
                                       const Version& version, const QString& author,
                                       const QString& name_en_US,
                                       const QString& description_en_US,
                                       const QString& keywords_en_US) throw (Exception) :
    QObject(nullptr), mXmlFilepath(), mXmlFileNamePrefix(xmlFileNamePrefix),
    mXmlRootNodeName(xmlRootNodeName), mDomTreeParsed(false),
    mUuid(uuid), mVersion(version), mAuthor(author),
    mCreated(QDateTime::currentDateTime()), mLastModified(QDateTime::currentDateTime())
{
    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
    mKeywords.insert("en_US", keywords_en_US);
}

LibraryBaseElement::LibraryBaseElement(const FilePath& xmlFilePath,
                                       const QString& xmlFileNamePrefix,
                                       const QString& xmlRootNodeName) throw (Exception) :
    QObject(0), mXmlFilepath(xmlFilePath), mXmlFileNamePrefix(xmlFileNamePrefix),
    mXmlRootNodeName(xmlRootNodeName), mDomTreeParsed(false)
{
}

LibraryBaseElement::~LibraryBaseElement() noexcept
{
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

void LibraryBaseElement::save(int version) const throw (Exception)
{
    Q_ASSERT(mXmlFilepath.isValid());
    XmlDomDocument doc(*serializeToXmlDomElement(version));
    QScopedPointer<SmartXmlFile> file(SmartXmlFile::create(mXmlFilepath));
    file->save(doc, true);
}

void LibraryBaseElement::saveTo(const FilePath& parentDir, int version) const throw (Exception)
{
    mXmlFilepath = parentDir.getPathTo(QString("%1/%2_v%3.xml")
        .arg(mUuid.toString()).arg(mXmlFileNamePrefix).arg(version));
    save(version);
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

void LibraryBaseElement::readFromFile() throw (Exception)
{
    Q_ASSERT(mDomTreeParsed == false);

    // open XML file
    SmartXmlFile file(mXmlFilepath, false, false);
    QSharedPointer<XmlDomDocument> doc = file.parseFileAndBuildDomTree(true);
    parseDomTree(doc->getRoot());
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    Q_ASSERT(mDomTreeParsed == true);
}

void LibraryBaseElement::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    Q_ASSERT(mDomTreeParsed == false);

    // read attributes
    mUuid = root.getFirstChild("meta/uuid", true, true)->getText<QUuid>();
    mVersion = root.getFirstChild("meta/version", true, true)->getText<Version>();

    // read names, descriptions and keywords in all available languages
    readLocaleDomNodes(*root.getFirstChild("meta", true), "name", mNames);
    readLocaleDomNodes(*root.getFirstChild("meta", true), "description", mDescriptions);
    readLocaleDomNodes(*root.getFirstChild("meta", true), "keywords", mKeywords);

    mDomTreeParsed = true;
}

XmlDomElement* LibraryBaseElement::serializeToXmlDomElement(int version) const throw (Exception)
{
    Q_UNUSED(version);

    bool valid = checkAttributesValidity();
    Q_ASSERT(valid == true);
    if (!valid) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement(mXmlRootNodeName));
    root->setAttribute("version", version);

    // meta
    XmlDomElement* meta = root->appendChild("meta");
    meta->appendTextChild("uuid", mUuid.toString());
    meta->appendTextChild("version", mVersion.toStr());
    meta->appendTextChild("author", mAuthor);
    meta->appendTextChild("created", mCreated.toUTC().toString(Qt::ISODate));
    meta->appendTextChild("last_modified", mLastModified.toUTC().toString(Qt::ISODate));
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
        QString locale = node->getAttribute("locale", true);
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
        list.insert(locale, node->getText());
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
