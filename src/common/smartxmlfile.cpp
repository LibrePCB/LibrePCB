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
#include "smartxmlfile.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartXmlFile::SmartXmlFile(const FilePath& filepath, bool restore, bool readOnly, bool create,
                 const QString& rootName, int expectedVersion, int createVersion) throw (Exception) :
    SmartFile(filepath, restore, readOnly, create), mDomDocument(), mDomRoot(),
    mFileVersion(-1)
{
    mDomDocument.implementation().setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    QString fileContent;

    if (mIsCreated)
    {
        QString xmlTmpl("<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\n<%1 file_version=\"%2\"/>");
        fileContent = xmlTmpl.arg(rootName).arg(createVersion).toUtf8();
    }
    else
    {
        // read the content of the file
        fileContent = readContentFromFile(mOpenedFilePath);
    }

    // load XML from mContent into mDomDocument
    QString errMsg;
    int errLine;
    int errColumn;
    if (!mDomDocument.setContent(fileContent, &errMsg, &errLine, &errColumn))
    {
        QString line = fileContent.split('\n').at(errLine-1);
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3:%4] LINE:%5")
            .arg(mOpenedFilePath.toStr(), errMsg).arg(errLine).arg(errColumn).arg(line),
            QString(tr("Error while parsing XML in file \"%1\": %2 [%3:%4]"))
            .arg(mOpenedFilePath.toNative(), errMsg).arg(errLine).arg(errColumn));
    }

    // check if the root node exists
    mDomRoot = mDomDocument.documentElement();
    if (mDomRoot.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mOpenedFilePath.toStr(),
            QString(tr("No XML root node found in \"%1\"!")).arg(mOpenedFilePath.toNative()));
    }

    // check the name of the root node, if desired
    if ((!rootName.isEmpty()) && (mDomRoot.tagName() != rootName))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: \"%2\"!=\"%3\"")
            .arg(mOpenedFilePath.toStr(), mDomRoot.nodeName(), rootName),
            QString(tr("Invalid root node in \"%1\"!")).arg(mOpenedFilePath.toNative()));
    }

    // read the file version
    bool ok;
    mFileVersion = mDomRoot.attribute("file_version").toInt(&ok);
    if (!ok) mFileVersion = -1;

    // check the file version number
    if ((expectedVersion > -1) && (mFileVersion != expectedVersion))
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Invalid file version in \"%1\": %2 (expected: %3)"))
            .arg(mOpenedFilePath.toNative()).arg(mFileVersion).arg(expectedVersion));
    }
}

SmartXmlFile::~SmartXmlFile() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SmartXmlFile::setFileVersion(int version) noexcept
{
    // Do NOT use QDomElement::setAttribute(QString, int) as it will use the user's locale!
    // Use the locale-independent QString::number(int) instead to convert the version number!
    mDomRoot.setAttribute("file_version", QString::number(version));
    mFileVersion = version;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartXmlFile* SmartXmlFile::create(const FilePath &filepath, const QString& rootName,
                                   int version) throw (Exception)
{
    return new SmartXmlFile(filepath, false, false, true, rootName, version, version);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void SmartXmlFile::saveToFile(const FilePath& filepath) throw (Exception)
{
    saveContentToFile(filepath, mDomDocument.toByteArray(4));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
