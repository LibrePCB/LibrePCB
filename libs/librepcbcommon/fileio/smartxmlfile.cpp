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
#include "smartxmlfile.h"
#include "xmldomdocument.h"
#include "xmldomelement.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartXmlFile::SmartXmlFile(const FilePath& filepath, bool restore, bool readOnly, bool create) throw (Exception) :
    SmartFile(filepath, restore, readOnly, create)
{
}

SmartXmlFile::~SmartXmlFile() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QSharedPointer<XmlDomDocument> SmartXmlFile::parseFileAndBuildDomTree(bool checkVersion) const throw (Exception)
{
    QSharedPointer<XmlDomDocument> doc(
        new XmlDomDocument(readContentFromFile(mOpenedFilePath), mOpenedFilePath));

    if (checkVersion)
    {
        int appVersion = APP_VERSION_MAJOR;
        int fileVersion = doc->getFileVersion();
        if (!(fileVersion <= appVersion))
        {
            throw RuntimeError(__FILE__, __LINE__, QString::number(appVersion),
                QString(tr("The file %1 was created with a newer application version. "
                           "You need at least version %2.0.0 to open this file."))
                .arg(mOpenedFilePath.toNative()).arg(fileVersion));
        }
    }

    return doc;
}

void SmartXmlFile::save(const XmlDomDocument& domDocument, bool toOriginal) throw (Exception)
{
    // check if file version <= application's major version
    Q_ASSERT(domDocument.getRoot().hasAttribute("version"));
    Q_ASSERT(domDocument.getFileVersion() >= 0);
    Q_ASSERT(domDocument.getFileVersion() <= APP_VERSION_MAJOR);

    const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);
    saveContentToFile(filepath, domDocument.toByteArray());
    updateMembersAfterSaving(toOriginal);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartXmlFile* SmartXmlFile::create(const FilePath &filepath) throw (Exception)
{
    return new SmartXmlFile(filepath, false, false, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
