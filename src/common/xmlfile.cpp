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
#include "xmlfile.h"
#include "exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

XmlFile::XmlFile(const FilePath& filepath, bool restore, bool readOnly,
                 const QString& rootName) throw (Exception) :
    TextFile(filepath, restore, readOnly), mDomDocument(), mDomRoot(), mFileVersion(-1)
{
    mDomDocument.implementation().setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    // load XML into mDomDocument
    QString errMsg;
    int errLine;
    int errColumn;
    if (!mDomDocument.setContent(mContent, &errMsg, &errLine, &errColumn))
    {
        QString line = mContent.split('\n').at(errLine-1);
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
    int version = mDomRoot.attribute("file_version").toInt(&ok);
    mFileVersion = ok ? version : -1;
}

XmlFile::~XmlFile() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void XmlFile::setFileVersion(int version) noexcept
{
    // Do NOT use QDomElement::setAttribute(QString, int) as it will use the user's locale!
    // Use the locale-independent QString::number(int) instead to convert the version number!
    mDomRoot.setAttribute("file_version", QString::number(version));
    mFileVersion = version;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void XmlFile::save(bool toOriginal) throw (Exception)
{
    if (mIsReadOnly)
        throw LogicError(__FILE__, __LINE__, QString(), tr("Cannot save read-only file!"));

    mContent = mDomDocument.toByteArray(4);
    TextFile::save(toOriginal);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

XmlFile* XmlFile::create(const FilePath& filepath, const QString& rootName, int version) throw (Exception)
{
    QString xmlTmpl("<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\n<%1/>");

    // create DOM document
    QDomDocument dom;
    QString errMsg;
    if (!dom.setContent(xmlTmpl.arg(rootName), &errMsg))
        throw LogicError(__FILE__, __LINE__, errMsg, tr("Could not set XML DOM content!"));
    QDomElement root = dom.documentElement();
    if ((root.isNull()) || (root.tagName() != rootName))
        throw LogicError(__FILE__, __LINE__, rootName, tr("No DOM root found!"));
    if (version > -1)
        root.setAttribute("version", QString::number(version)); // see comment in #setFileVersion()

    // save DOM document to temporary file
    saveContentToFile(FilePath(filepath.toStr() % '~'), dom.toByteArray(4));

    return new XmlFile(filepath, true, false, rootName);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
