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
#include "domelement.h"
#include "domdocument.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DomElement::DomElement(const QString& name, const QString& text) noexcept :
    mDocument(nullptr), mParent(nullptr), mName(name), mText(text)
{
    Q_ASSERT(isValidTagName(mName) == true);
}

DomElement::DomElement(QDomElement domElement, DomElement* parent, DomDocument* doc) noexcept :
    mDocument(doc), mParent(parent), mName(domElement.tagName()), mText()
{
    Q_ASSERT(isValidTagName(mName) == true);

    QDomNamedNodeMap map = domElement.attributes();
    for (int i = 0; i < map.count(); i++)
        mAttributes.insert(map.item(i).nodeName(), map.item(i).nodeValue());

    QDomElement child = domElement.firstChildElement();
    while (!child.isNull())
    {
        mChilds.append(new DomElement(child, this));
        child = child.nextSiblingElement();
    }

    if (mChilds.isEmpty())
        mText = domElement.text();
}

DomElement::~DomElement() noexcept
{
    qDeleteAll(mChilds);        mChilds.clear();

    if (mParent)
        mParent->removeChild(this, false);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

DomDocument* DomElement::getDocument(bool docOfTree) const noexcept
{
    if (mParent && docOfTree)
        return mParent->getDocument(docOfTree);
    else
        return mDocument;
}

void DomElement::setDocument(DomDocument* doc) noexcept
{
    Q_ASSERT((mParent == nullptr) || (doc == nullptr));
    mDocument = doc;
}

FilePath DomElement::getDocFilePath() const noexcept
{
    DomDocument* doc = getDocument(true);
    if (doc)
        return doc->getFilePath();
    else
        return FilePath();
}

/*****************************************************************************************
 *  Attribute Handling Methods
 ****************************************************************************************/

bool DomElement::hasAttribute(const QString& name) const noexcept
{
    return mAttributes.contains(name);
}

/*****************************************************************************************
 *  Child Handling Methods
 ****************************************************************************************/

QList<DomElement*> DomElement::getChilds(const QString& name) const noexcept
{
    QList<DomElement*> childs;
    foreach (DomElement* child, mChilds) {
        if (child->getName() == name) {
            childs.append(child);
        }
    }
    return childs;
}

void DomElement::removeChild(DomElement* child, bool deleteChild) noexcept
{
    Q_ASSERT(child);
    Q_ASSERT(mChilds.contains(child) == true);
    mChilds.removeOne(child);
    if (deleteChild) {
        delete child;
    } else {
        child->mParent = nullptr;
    }
}

void DomElement::appendChild(DomElement* child) noexcept
{
    Q_ASSERT(mText.isNull() == true);
    Q_ASSERT(child);
    Q_ASSERT(mChilds.contains(child) == false);
    Q_ASSERT(child->mDocument == nullptr);
    Q_ASSERT(child->mParent == nullptr);
    child->mParent = this;
    mChilds.append(child);
}

DomElement* DomElement::appendChild(const QString& name) noexcept
{
    QScopedPointer<DomElement> child(new DomElement(name));
    appendChild(child.data());
    return child.take();
}

DomElement* DomElement::getFirstChild(bool throwIfNotFound) const throw (Exception)
{
    if (!mChilds.isEmpty())
        return mChilds.first();
    else if (!throwIfNotFound)
        return nullptr;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
            QString(tr("No child in node \"%1\" found.")).arg(mName));
    }
}

DomElement* DomElement::getFirstChild(const QString& name, bool throwIfNotFound) const throw (Exception)
{
    foreach (DomElement* child, mChilds)
    {
        if (child->getName() == name)
            return child;
    }
    if (!throwIfNotFound)
        return nullptr;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
            QString(tr("Child \"%1\" in node \"%2\" not found.")).arg(name, mName));
    }
}

DomElement* DomElement::getFirstChild(const QString& pathName, bool throwIfPathNotExist,
                                            bool throwIfChildNotFound) const throw (Exception)
{
    int separatorPos = pathName.indexOf("/");
    if (separatorPos > -1)
    {
        DomElement* child = getFirstChild(pathName.left(separatorPos), throwIfPathNotExist);
        if (!child) return nullptr;
        return child->getFirstChild(pathName.right(pathName.length() - separatorPos - 1),
                                    throwIfPathNotExist, throwIfChildNotFound);
    }
    else
    {
        if (pathName == "*")
            return getFirstChild(throwIfChildNotFound);
        else
            return getFirstChild(pathName, throwIfChildNotFound);
    }
}

DomElement* DomElement::getPreviousChild(const DomElement* child, const QString& name,
                                               bool throwIfNotFound) const throw (Exception)
{
    DomElement* previousChild = const_cast<DomElement*>(child);
    do
    {
        int index = mChilds.indexOf(const_cast<DomElement*>(previousChild));
        Q_ASSERT(index > -1);
        if (index > 0)
            previousChild = mChilds.at(index-1);
        else if (!throwIfNotFound)
            return nullptr;
        else
        {
            throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
                QString(tr("Child \"%1\" of node \"%2\" not found.")).arg(name, mName));
        }
    } while ((previousChild->getName() != name) && (!name.isNull()));
    return previousChild;
}

DomElement* DomElement::getNextChild(const DomElement* child, const QString& name,
                                           bool throwIfNotFound) const throw (Exception)
{
    DomElement* nextChild = const_cast<DomElement*>(child);
    do
    {
        int index = mChilds.indexOf(const_cast<DomElement*>(nextChild));
        Q_ASSERT(index > -1);
        if (index < mChilds.count()-1)
            nextChild = mChilds.at(index+1);
        else if (!throwIfNotFound)
            return nullptr;
        else
        {
            throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
                QString(tr("Child \"%1\" of node \"%2\" not found.")).arg(name, mName));
        }
    } while ((nextChild->getName() != name) && (!name.isNull()));
    return nextChild;
}

/*****************************************************************************************
 *  Sibling Handling Methods
 ****************************************************************************************/

DomElement* DomElement::getPreviousSibling(const QString& name, bool throwIfNotFound) const throw (Exception)
{
    if (mParent)
        return mParent->getPreviousChild(this, name, throwIfNotFound);
    else if (!throwIfNotFound)
        return nullptr;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
            QString(tr("Sibling \"%1\" of node \"%2\" not found.")).arg(name, mName));
    }
}

DomElement* DomElement::getNextSibling(const QString& name, bool throwIfNotFound) const throw (Exception)
{
    if (mParent)
        return mParent->getNextChild(this, name, throwIfNotFound);
    else if (!throwIfNotFound)
        return nullptr;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
            QString(tr("Sibling \"%1\" of node \"%2\" not found.")).arg(name, mName));
    }
}

/*****************************************************************************************
 *  QDomElement Converter Methods
 ****************************************************************************************/

void DomElement::writeToQXmlStreamWriter(QXmlStreamWriter& writer) const noexcept
{
    writer.writeStartElement(mName);
    foreach (const QString& key, mAttributes.keys()) {
        writer.writeAttribute(key, mAttributes[key]);
    }
    if (hasChilds()) {
        foreach (DomElement* child, mChilds) {
            child->writeToQXmlStreamWriter(writer);
        }
    } else if (!mText.isNull()) {
        writer.writeCharacters(mText);
    }
    writer.writeEndElement();
}

DomElement* DomElement::fromQDomElement(QDomElement domElement, DomDocument* doc) noexcept
{
    return new DomElement(domElement, nullptr, doc);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool DomElement::isValidTagName(const QString& name) noexcept
{
    bool valid = !name.isEmpty();
    if (name.startsWith("xml", Qt::CaseInsensitive)) valid = false;
    for (int i=0; i<name.length(); i++)
    {
        bool char_valid = false;
        ushort value = name.at(i).unicode();
        if ((i > 0) && (value >= '0') && (value <= '9')) char_valid = true;
        if ((i > 0) && (value == '_')) char_valid = true;
        if ((value >= 'A') && (value <= 'Z')) char_valid = true;
        if ((value >= 'a') && (value <= 'z')) char_valid = true;
        if (!char_valid) valid = false;
    }
    return valid;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
