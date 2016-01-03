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
#include <librepcbcommon/fileio/xmldomelement.h>
#include "packagepad.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackagePad::PackagePad(const Uuid& uuid, const QString& name) noexcept :
    mUuid(uuid), mName(name)
{
    Q_ASSERT(mUuid.isNull() == false);
}

PackagePad::PackagePad(const XmlDomElement& domElement) throw (Exception) :
    mUuid()
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mName = domElement.getText<QString>(true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

PackagePad::~PackagePad() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void PackagePad::setName(const QString& name) noexcept
{
    mName = name;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* PackagePad::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("pad"));
    root->setAttribute("uuid", mUuid);
    root->setText(mName);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool PackagePad::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
