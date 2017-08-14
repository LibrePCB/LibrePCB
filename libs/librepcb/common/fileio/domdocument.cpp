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
#include "domdocument.h"
#include "domelement.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DomDocument::DomDocument(DomElement& root) noexcept :
    mFilePath(), mRootElement(&root)
{
    mRootElement->setDocument(this);
}

DomDocument::DomDocument(const QByteArray& fileContent, const FilePath& filepath) :
    mFilePath(filepath), mRootElement(nullptr)
{
    QDomDocument doc;
    doc.implementation().setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    QString errMsg;
    int errLine;
    int errColumn;
    if (!doc.setContent(fileContent, &errMsg, &errLine, &errColumn)) {
        qDebug() << "line:" << fileContent.split('\n').at(errLine-1);
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Error while parsing file \"%1\": %2 [%3:%4]"))
            .arg(filepath.toNative(), errMsg).arg(errLine).arg(errColumn));
    }

    // check if the root node exists
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("No root node found in \"%1\"!")).arg(mFilePath.toNative()));
    }

    mRootElement.reset(DomElement::fromQDomElement(root, this));
}

DomDocument::~DomDocument() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

DomElement& DomDocument::getRoot(const QString& expectedName) const
{
    DomElement& root = getRoot();
    if (root.getName() != expectedName) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Root node name mismatch in file \"%1\": %2 != %3"))
            .arg(mFilePath.toNative(), root.getName(), expectedName));
    }
    return root;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QByteArray DomDocument::toByteArray() const
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
