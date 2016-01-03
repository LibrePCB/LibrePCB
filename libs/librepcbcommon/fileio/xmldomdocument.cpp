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
#include "xmldomdocument.h"
#include "xmldomelement.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

XmlDomDocument::XmlDomDocument(XmlDomElement& root) noexcept :
    mFilePath(), mRootElement(&root)
{
    mRootElement->setDocument(this);
}

XmlDomDocument::XmlDomDocument(const QByteArray& xmlFileContent, const FilePath& filepath) throw (Exception) :
    mFilePath(filepath), mRootElement(nullptr)
{
    QDomDocument doc;
    doc.implementation().setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    QString errMsg;
    int errLine;
    int errColumn;
    if (!doc.setContent(xmlFileContent, &errMsg, &errLine, &errColumn))
    {
        QString line = xmlFileContent.split('\n').at(errLine-1);
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3:%4] LINE:%5")
            .arg(filepath.toStr(), errMsg).arg(errLine).arg(errColumn).arg(line),
            QString(tr("Error while parsing XML in file \"%1\": %2 [%3:%4]"))
            .arg(filepath.toNative(), errMsg).arg(errLine).arg(errColumn));
    }

    // check if the root node exists
    QDomElement root = doc.documentElement();
    if (root.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("No XML root node found in \"%1\"!")).arg(/*xmlFilePath.toNative()*/QString()));
    }

    mRootElement = XmlDomElement::fromQDomElement(root, this);
}

XmlDomDocument::~XmlDomDocument() noexcept
{
    delete mRootElement;        mRootElement = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool XmlDomDocument::hasFileVersion() const noexcept
{
    Q_ASSERT(mRootElement != nullptr);
    return mRootElement->hasAttribute("version");
}

int XmlDomDocument::getFileVersion() const throw (Exception)
{
    Q_ASSERT(mRootElement != nullptr);
    return mRootElement->getAttribute<uint>("version", true); // use "uint" to automatically check for >= 0
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void XmlDomDocument::setFileVersion(int version) noexcept
{
    Q_ASSERT(version >= 0);
    Q_ASSERT(version <= APP_VERSION_MAJOR);
    Q_ASSERT(mRootElement != nullptr);
    mRootElement->setAttribute("version", version);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QByteArray XmlDomDocument::toByteArray() const noexcept
{
    QDomDocument doc;
    doc.implementation().setInvalidDataPolicy(QDomImplementation::ReturnNullNode);
    doc.setContent(QString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"));
    doc.appendChild(mRootElement->toQDomElement(doc));
    return doc.toByteArray(1); // indent only 1 space to save disk space
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
