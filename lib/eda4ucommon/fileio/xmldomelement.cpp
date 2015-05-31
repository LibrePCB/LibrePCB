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
#include "xmldomelement.h"
#include "xmldomdocument.h"
#include "../units/all_length_units.h"
#include "../version.h"
#include "../alignment.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

XmlDomElement::XmlDomElement(const QString& name, const QString& text) noexcept :
    mDocument(nullptr), mParent(nullptr), mName(name), mText(text)
{
    Q_ASSERT(isValidXmlTagName(mName) == true);
}

XmlDomElement::XmlDomElement(QDomElement domElement, XmlDomElement* parent, XmlDomDocument* doc) noexcept :
    mDocument(doc), mParent(parent), mName(domElement.tagName()), mText()
{
    Q_ASSERT(isValidXmlTagName(mName) == true);

    QDomNamedNodeMap map = domElement.attributes();
    for (int i = 0; i < map.count(); i++)
        mAttributes.insert(map.item(i).nodeName(), map.item(i).nodeValue());

    QDomElement child = domElement.firstChildElement();
    while (!child.isNull())
    {
        mChilds.append(new XmlDomElement(child, this));
        child = child.nextSiblingElement();
    }

    if (mChilds.isEmpty())
        mText = domElement.text();
}

XmlDomElement::~XmlDomElement() noexcept
{
    qDeleteAll(mChilds);        mChilds.clear();

    if (mParent)
        mParent->removeChild(this, false);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomDocument* XmlDomElement::getDocument(bool docOfTree) const noexcept
{
    if (mParent && docOfTree)
        return mParent->getDocument(docOfTree);
    else
        return mDocument;
}

void XmlDomElement::setDocument(XmlDomDocument* doc) noexcept
{
    Q_ASSERT((mParent == nullptr) || (doc == nullptr));
    mDocument = doc;
}

FilePath XmlDomElement::getDocFilePath() const noexcept
{
    XmlDomDocument* doc = getDocument(true);
    if (doc)
        return doc->getFilePath();
    else
        return FilePath();
}

/*****************************************************************************************
 *  Text Handling Methods
 ****************************************************************************************/

void XmlDomElement::setText(const QString& text) noexcept
{
    Q_ASSERT(mChilds.isEmpty() == true);
    mText = text;
}

const QString& XmlDomElement::getText(bool throwIfEmpty) const throw (Exception)
{
    if (hasChilds())
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, mName,
                             tr("A node with child elements cannot have a text."));
    }
    if (mText.isEmpty() && throwIfEmpty)
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, mName,
                             tr("The node text must not be empty."));
    }
    return mText;
}

/// @cond SPECIALIZED_TEMPLATE_IMPLELEMTATIONS

template <>
bool XmlDomElement::getText<bool>(bool throwIfEmpty, const bool& defaultValue) const throw (Exception)
{
    QString text = getText(throwIfEmpty);
    if (text == "true")
        return true;
    else if (text == "false")
        return false;
    else if ((text.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, text,
                             QString(tr("Invalid boolean value in node \"%1\".")).arg(mName));
    }
}

template <>
QUuid XmlDomElement::getText<QUuid>(bool throwIfEmpty, const QUuid& defaultValue) const throw (Exception)
{
    QString text = getText(throwIfEmpty);
    QUuid obj(text);
    if (!obj.isNull())
        return obj;
    else if ((text.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, text,
                             QString(tr("Invalid UUID in node \"%1\".")).arg(mName));
    }
}

template <>
QDateTime XmlDomElement::getText<QDateTime>(bool throwIfEmpty, const QDateTime& defaultValue) const throw (Exception)
{
    QString text = getText(throwIfEmpty);
    QDateTime obj = QDateTime::fromString(text, Qt::ISODate).toLocalTime();
    if (obj.isValid())
        return obj;
    else if ((text.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, text,
                             QString(tr("Invalid date/time in node \"%1\".")).arg(mName));
    }
}

template <>
Version XmlDomElement::getText<Version>(bool throwIfEmpty, const Version& defaultValue) const throw (Exception)
{
    QString text = getText(throwIfEmpty);
    Version obj(text);
    if (obj.isValid())
        return obj;
    else if ((text.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, text,
                             QString(tr("Invalid version number in node \"%1\".")).arg(mName));
    }
}

/// @endcond

/*****************************************************************************************
 *  Attribute Handling Methods
 ****************************************************************************************/

/// @cond SPECIALIZED_TEMPLATE_IMPLELEMTATIONS

template <>
void XmlDomElement::setAttribute(const QString& name, const QString& value) noexcept
{
    mAttributes.insert(name, value);
}

template <>
void XmlDomElement::setAttribute(const QString& name, const bool& value) noexcept
{
    setAttribute<QString>(name, value ? "true" : "false");
}

template <>
void XmlDomElement::setAttribute(const QString& name, const char* const& value) noexcept
{
    setAttribute<QString>(name, value);
}

template <>
void XmlDomElement::setAttribute(const QString& name, const int& value) noexcept
{
    setAttribute<QString>(name, QString::number(value));
}

template <>
void XmlDomElement::setAttribute(const QString& name, const uint& value) noexcept
{
    setAttribute<QString>(name, QString::number(value));
}

template <>
void XmlDomElement::setAttribute(const QString& name, const QUuid& value) noexcept
{
    setAttribute<QString>(name, value.isNull() ? "" : value.toString());
}

template <>
void XmlDomElement::setAttribute(const QString& name, const Length& value) noexcept
{
    setAttribute<QString>(name, value.toMmString());
}

template <>
void XmlDomElement::setAttribute(const QString& name, const Angle& value) noexcept
{
    setAttribute<QString>(name, value.toDegString());
}

template <>
void XmlDomElement::setAttribute(const QString& name, const HAlign& value) noexcept
{
    setAttribute<QString>(name, value.toString());
}

template <>
void XmlDomElement::setAttribute(const QString& name, const VAlign& value) noexcept
{
    setAttribute<QString>(name, value.toString());
}

/// @endcond

bool XmlDomElement::hasAttribute(const QString& name) const noexcept
{
    return mAttributes.contains(name);
}

QString XmlDomElement::getAttribute(const QString& name, bool throwIfEmpty) const throw (Exception)
{
    if (!mAttributes.contains(name))
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
            QString(tr("Attribute \"%1\" not found in node \"%2\".")).arg(name, mName));
    }
    if (mAttributes.value(name).isEmpty() && throwIfEmpty)
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, QString(),
            QString(tr("Attribute \"%1\" in node \"%2\" must not be empty.")).arg(name, mName));
    }
    return mAttributes.value(name);
}

/// @cond SPECIALIZED_TEMPLATE_IMPLELEMTATIONS

template <>
bool XmlDomElement::getAttribute<bool>(const QString& name, bool throwIfEmpty, const bool& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    if (attr == "true")
        return true;
    else if (attr == "false")
        return false;
    else if ((attr.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
            QString(tr("Invalid boolean attribute \"%1\" in node \"%2\".")).arg(name, mName));
    }
}

template <>
uint XmlDomElement::getAttribute<uint>(const QString& name, bool throwIfEmpty, const uint& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    bool ok = false;
    uint value = attr.toUInt(&ok);
    if (ok)
        return value;
    else if ((attr.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr, QString(tr(
            "Invalid unsigned integer attribute \"%1\" in node \"%2\".")).arg(name, mName));
    }
}

template <>
int XmlDomElement::getAttribute<int>(const QString& name, bool throwIfEmpty, const int& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    bool ok = false;
    int value = attr.toInt(&ok);
    if (ok)
        return value;
    else if ((attr.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
            QString(tr("Invalid integer attribute \"%1\" in node \"%2\".")).arg(name, mName));
    }
}

template <>
QUuid XmlDomElement::getAttribute<QUuid>(const QString& name, bool throwIfEmpty, const QUuid& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    QUuid obj(attr);
    if (!obj.isNull())
        return obj;
    else if ((attr.isEmpty()) && (!throwIfEmpty))
        return defaultValue;
    else
    {
        throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
            QString(tr("Invalid UUID attribute \"%1\" in node \"%2\".")).arg(name, mName));
    }
}

template <>
Length XmlDomElement::getAttribute<Length>(const QString& name, bool throwIfEmpty, const Length& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    try
    {
        Length obj = Length::fromMm(attr);
        return obj;
    }
    catch (Exception& exc)
    {
        if ((attr.isEmpty()) && (!throwIfEmpty))
            return defaultValue;
        else
        {
            throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
                QString(tr("Invalid length attribute \"%1\" in node \"%2\".")).arg(name, mName));
        }
    }
}

template <>
Angle XmlDomElement::getAttribute<Angle>(const QString& name, bool throwIfEmpty, const Angle& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    try
    {
        Angle obj = Angle::fromDeg(attr);
        return obj;
    }
    catch (Exception& exc)
    {
        if ((attr.isEmpty()) && (!throwIfEmpty))
            return defaultValue;
        else
        {
            throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
                QString(tr("Invalid angle attribute \"%1\" in node \"%2\".")).arg(name, mName));
        }
    }
}

template <>
HAlign XmlDomElement::getAttribute<HAlign>(const QString& name, bool throwIfEmpty, const HAlign& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    try
    {
        HAlign obj = HAlign::fromString(attr);
        return obj;
    }
    catch (Exception& exc)
    {
        if ((attr.isEmpty()) && (!throwIfEmpty))
            return defaultValue;
        else
        {
            throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
                QString(tr("Invalid horizontal align attribute \"%1\" in node \"%2\".")).arg(name, mName));
        }
    }
}

template <>
VAlign XmlDomElement::getAttribute<VAlign>(const QString& name, bool throwIfEmpty, const VAlign& defaultValue) const throw (Exception)
{
    QString attr = getAttribute(name, throwIfEmpty);
    try
    {
        VAlign obj = VAlign::fromString(attr);
        return obj;
    }
    catch (Exception& exc)
    {
        if ((attr.isEmpty()) && (!throwIfEmpty))
            return defaultValue;
        else
        {
            throw FileParseError(__FILE__, __LINE__, getDocFilePath(), -1, -1, attr,
                QString(tr("Invalid vertical align attribute \"%1\" in node \"%2\".")).arg(name, mName));
        }
    }
}

/// @endcond

/*****************************************************************************************
 *  Child Handling Methods
 ****************************************************************************************/

void XmlDomElement::removeChild(XmlDomElement* child, bool deleteChild) noexcept
{
    Q_ASSERT(child);
    Q_ASSERT(mChilds.contains(child) == true);
    mChilds.removeOne(child);
    child->mDocument = nullptr;
    child->mParent = nullptr;
    if (deleteChild)
        delete child;
}

void XmlDomElement::appendChild(XmlDomElement* child) noexcept
{
    Q_ASSERT(mText.isNull() == true);
    Q_ASSERT(child);
    Q_ASSERT(mChilds.contains(child) == false);
    Q_ASSERT(child->mDocument == nullptr);
    Q_ASSERT(child->mParent == nullptr);
    mChilds.append(child);
}

XmlDomElement* XmlDomElement::appendChild(const QString& name) noexcept
{
    XmlDomElement* child = new XmlDomElement(name);
    appendChild(child);
    return child;
}

/// @cond SPECIALIZED_TEMPLATE_IMPLELEMTATIONS

template <>
XmlDomElement* XmlDomElement::appendTextChild(const QString& name, const QString& value) noexcept
{
    Q_ASSERT(mText.isNull() == true);
    XmlDomElement* child = new XmlDomElement(name, value);
    mChilds.append(child);
    child->mParent = this;
    return child;
}

template <>
XmlDomElement* XmlDomElement::appendTextChild(const QString& name, const bool& value) noexcept
{
    return appendTextChild<QString>(name, value ? "true" : "false");
}

template <>
XmlDomElement* XmlDomElement::appendTextChild(const QString& name, const QDateTime& value) noexcept
{
    return appendTextChild<QString>(name, value.toUTC().toString(Qt::ISODate));
}

template <>
XmlDomElement* XmlDomElement::appendTextChild(const QString& name, const QUuid& value) noexcept
{
    return appendTextChild<QString>(name, value.isNull() ? "" : value.toString());
}

/// @endcond

XmlDomElement* XmlDomElement::getFirstChild(bool throwIfNotFound) const throw (Exception)
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

XmlDomElement* XmlDomElement::getFirstChild(const QString& name, bool throwIfNotFound) const throw (Exception)
{
    foreach (XmlDomElement* child, mChilds)
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

XmlDomElement* XmlDomElement::getFirstChild(const QString& pathName, bool throwIfPathNotExist,
                                            bool throwIfChildNotFound) const throw (Exception)
{
    int separatorPos = pathName.indexOf("/");
    if (separatorPos > -1)
    {
        XmlDomElement* child = getFirstChild(pathName.left(separatorPos), throwIfPathNotExist);
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

XmlDomElement* XmlDomElement::getPreviousChild(const XmlDomElement* child, const QString& name,
                                               bool throwIfNotFound) const throw (Exception)
{
    XmlDomElement* previousChild = const_cast<XmlDomElement*>(child);
    do
    {
        int index = mChilds.indexOf(const_cast<XmlDomElement*>(previousChild));
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

XmlDomElement* XmlDomElement::getNextChild(const XmlDomElement* child, const QString& name,
                                           bool throwIfNotFound) const throw (Exception)
{
    XmlDomElement* nextChild = const_cast<XmlDomElement*>(child);
    do
    {
        int index = mChilds.indexOf(const_cast<XmlDomElement*>(nextChild));
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

XmlDomElement* XmlDomElement::getPreviousSibling(const QString& name, bool throwIfNotFound) const throw (Exception)
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

XmlDomElement* XmlDomElement::getNextSibling(const QString& name, bool throwIfNotFound) const throw (Exception)
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

QDomElement XmlDomElement::toQDomElement(QDomDocument& domDocument) const noexcept
{
    QDomElement element = domDocument.createElement(mName);

    if (hasChilds())
    {
        foreach (XmlDomElement* child, mChilds)
            element.appendChild(child->toQDomElement(domDocument));
    }
    else if (!mText.isNull())
    {
        QDomText textNode = domDocument.createTextNode(mText);
        element.appendChild(textNode);
    }

    foreach (const QString& key, mAttributes.keys())
    {
        element.setAttribute(key, mAttributes.value(key));
    }

    return element;
}

XmlDomElement* XmlDomElement::fromQDomElement(QDomElement domElement, XmlDomDocument* doc) noexcept
{
    return new XmlDomElement(domElement, nullptr, doc);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool XmlDomElement::isValidXmlTagName(const QString& name) noexcept
{
    bool valid = !name.isEmpty();
    for (int i=0; i<name.length(); i++)
    {
        ushort value = name.at(i).unicode();
        if (value < 'A')
            valid = false;
        else if ((value > 'Z') && (value < 'a') && (value != '_'))
            valid = false;
        else if (value > 'z')
            valid = false;
    }
    return valid;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
