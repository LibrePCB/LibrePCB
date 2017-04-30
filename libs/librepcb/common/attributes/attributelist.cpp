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
#include "attributelist.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AttributeList::AttributeList() noexcept
{
}

AttributeList::AttributeList(const AttributeList& other) noexcept
{
    foreach (const QSharedPointer<Attribute>& attr, other.mAttributes) {
        mAttributes.append(QSharedPointer<Attribute>::create(*attr));
    }
}

AttributeList::AttributeList(const XmlDomElement& domElement) throw (Exception)
{
    foreach (const XmlDomElement* node, domElement.getChilds()) {
        QSharedPointer<Attribute> attr = QSharedPointer<Attribute>::create(*node); // can throw
        if (value(attr->getKey())) {
            throw RuntimeError(__FILE__, __LINE__, attr->getKey(),
                QString(tr("The attribute \"%1\" exists multiple times in \"%2\"."))
                .arg(attr->getKey(), domElement.getDocFilePath().toNative()));
        }
        mAttributes.append(attr);
    }
}

AttributeList::~AttributeList() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

int AttributeList::indexOf(const QString& key) const noexcept
{
    for (int i = 0; i < mAttributes.count(); ++i) {
        if (mAttributes.at(i)->getKey() == key) {
            return i;
        }
    }
    return -1;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void AttributeList::serialize(XmlDomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    serializePointerContainer(root, mAttributes, "attribute");
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool AttributeList::operator==(const AttributeList& rhs) const noexcept
{
    if (mAttributes.count() != rhs.mAttributes.count()) {
        return false;
    }
    for (int i = 0; i < mAttributes.count(); ++i) {
        if (*mAttributes.at(i) != *rhs.mAttributes.at(i)) {
            return false;
        }
    }
    return true;
}

AttributeList& AttributeList::operator=(const AttributeList& rhs) noexcept
{
    mAttributes.clear();
    foreach (const QSharedPointer<Attribute>& attr, rhs.mAttributes) {
        mAttributes.append(QSharedPointer<Attribute>::create(*attr));
    }
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool AttributeList::checkAttributesValidity() const noexcept
{
    QStringList keys;
    foreach (const QSharedPointer<Attribute>& attr, mAttributes) {
        if (keys.contains(attr->getKey())) {
            return false;
        } else {
            keys.append(attr->getKey());
        }
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
