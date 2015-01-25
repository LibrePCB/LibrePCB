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
#include <QDomDocument>
#include <QDomElement>
#include "librarybaseelement.h"
#include "../workspace/workspace.h"
#include "../workspace/settings/workspacesettings.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryBaseElement::LibraryBaseElement(const FilePath& xmlFilePath,
                                       const QString& xmlRootNodeName) throw (Exception) :
    QObject(0), mXmlFilepath(xmlFilePath), mXmlFile(0)
{
    try
    {
        // open XML file
        mXmlFile = new SmartXmlFile(mXmlFilepath, false, false, xmlRootNodeName);
        mDomRoot = mXmlFile->getRoot();

        // read UUID
        mUuid = mDomRoot.firstChildElement("meta").firstChildElement("uuid").text();
        if (mUuid.isNull())
        {
            throw RuntimeError(__FILE__, __LINE__, mXmlFilepath.toStr(),
                QString(tr("Invalid UUID in file \"%1\".")).arg(mXmlFilepath.toNative()));
        }

        // read version
        QString version = mDomRoot.firstChildElement("meta").firstChildElement("version").text();
        if (!mVersion.setVersion(version))
        {
            throw RuntimeError(__FILE__, __LINE__, version,
                QString(tr("Invalid version in file \"%1\".")).arg(mXmlFilepath.toNative()));
        }

        // read names, descriptions and keywords in all available languages
        readLocaleDomNodes(mXmlFilepath, mDomRoot.firstChildElement("meta"), "name", mNames);
        readLocaleDomNodes(mXmlFilepath, mDomRoot.firstChildElement("meta"), "description", mDescriptions);
        readLocaleDomNodes(mXmlFilepath, mDomRoot.firstChildElement("meta"), "keywords", mKeywords);
    }
    catch (Exception& e)
    {
        delete mXmlFile;        mXmlFile = 0;
        throw;
    }
}

LibraryBaseElement::~LibraryBaseElement() noexcept
{
    delete mXmlFile;        mXmlFile = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString LibraryBaseElement::getName(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, locale);
}

QString LibraryBaseElement::getDescription(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, locale);
}

QString LibraryBaseElement::getKeywords(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mKeywords, locale);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

void LibraryBaseElement::readLocaleDomNodes(const FilePath& xmlFilepath, QDomElement parentNode,
                                            const QString& childNodesName,
                                            QHash<QString, QString>& list) throw (Exception)
{
    parentNode = parentNode.firstChildElement(childNodesName);
    while (!parentNode.isNull())
    {
        QString locale = parentNode.attribute("locale");
        if (locale.isEmpty())
        {
            throw RuntimeError(__FILE__, __LINE__, xmlFilepath.toStr(),
                QString(tr("Entry without locale found in \"%1\"."))
                .arg(xmlFilepath.toNative()));
        }
        if (list.contains(locale))
        {
            throw RuntimeError(__FILE__, __LINE__, xmlFilepath.toStr(),
                QString(tr("Locale \"%1\" defined multiple times in \"%2\"."))
                .arg(locale, xmlFilepath.toNative()));
        }
        list.insert(locale, parentNode.text());
        parentNode = parentNode.nextSiblingElement(childNodesName);
    }
    if (!list.contains("en_US"))
    {
        throw RuntimeError(__FILE__, __LINE__, xmlFilepath.toStr(), QString(
            tr("At least one entry in \"%1\" has no translation for locale \"en_US\"."))
            .arg(xmlFilepath.toNative()));
    }
}

QString LibraryBaseElement::localeStringFromList(const QHash<QString, QString>& list,
                                                 const QString& locale, QString* usedLocale) throw (Exception)
{
    if ((!locale.isEmpty()) && (list.contains(locale)))
    {
        if (usedLocale) *usedLocale = locale;
        return list.value(locale);
    }

    foreach (const QString& l, Workspace::instance().getSettings().getLibLocaleOrder()->getLocaleOrder())
    {
        if (list.contains(l))
        {
            if (usedLocale) *usedLocale = l;
            return list.value(l);
        }
    }

    if (list.contains("en_US"))
    {
        if (usedLocale) *usedLocale = "en_US";
        return list.value("en_US");
    }

    throw RuntimeError(__FILE__, __LINE__, locale,
        QString(tr("No translation for locale \"%1\" found.")).arg(locale));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
