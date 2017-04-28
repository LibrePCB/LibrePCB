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
#include "xmldomdocument.h"
#include "xmldomelement.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

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
    if (!doc.setContent(xmlFileContent, &errMsg, &errLine, &errColumn)) {
        QString line = xmlFileContent.split('\n').at(errLine-1);
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3:%4] LINE:%5")
            .arg(filepath.toStr(), errMsg).arg(errLine).arg(errColumn).arg(line),
            QString(tr("Error while parsing XML in file \"%1\": %2 [%3:%4]"))
            .arg(filepath.toNative(), errMsg).arg(errLine).arg(errColumn));
    }

    // check if the root node exists
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("No XML root node found in \"%1\"!")).arg(mFilePath.toNative()));
    }

    mRootElement.reset(XmlDomElement::fromQDomElement(root, this));
}

XmlDomDocument::~XmlDomDocument() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

XmlDomElement& XmlDomDocument::getRoot(const QString& expectedName) const throw (Exception)
{
    XmlDomElement& root = getRoot();
    if (root.getName() != expectedName) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("XML root node name mismatch in file \"%1\": %2 != %3"))
            .arg(mFilePath.toNative(), root.getName(), expectedName));
    }
    return root;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QByteArray XmlDomDocument::toByteArray() const throw (Exception)
{
    QByteArray data;
    QXmlStreamWriter writer(&data);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1); // indent only 1 space to save disk space
    writer.setCodec("UTF-8");
    writer.writeStartDocument("1.0", true);
    mRootElement->writeToQXmlStreamWriter(writer);
    writer.writeEndDocument();
    if (writer.hasError()) throw LogicError(__FILE__, __LINE__);
    return data;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
