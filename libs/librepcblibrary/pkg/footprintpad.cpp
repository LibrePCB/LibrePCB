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
#include <librepcbcommon/boardlayer.h>
#include "footprintpad.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPad::FootprintPad(const QUuid& padUuid) noexcept :
    mPadUuid(padUuid), mType(Type_t::ThtRect), mPosition(0, 0), mRotation(0), mWidth(0),
    mHeight(0), mDrillDiameter(0), mLayerId(0)
{
}

FootprintPad::FootprintPad(const XmlDomElement& domElement) throw (Exception) :
    mPadUuid(), mPosition()
{
    // read attributes
    mPadUuid = domElement.getAttribute<QUuid>("uuid");
    mType = stringToType(domElement.getAttribute("type"));
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mRotation = domElement.getAttribute<Angle>("rotation");
    mWidth = domElement.getAttribute<Length>("width");
    mHeight = domElement.getAttribute<Length>("height");
    mDrillDiameter = domElement.getAttribute<Length>("tht_drill");
    if (domElement.getAttribute("smt_side", true) == "bottom")
        mLayerId = BoardLayer::BottomCopper;
    else
        mLayerId = BoardLayer::TopCopper;

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

FootprintPad::~FootprintPad() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* FootprintPad::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("pad"));
    root->setAttribute("uuid", mPadUuid);
    root->setAttribute("type", typeToString(mType));
    root->setAttribute("x", mPosition.getX().toMmString());
    root->setAttribute("y", mPosition.getY().toMmString());
    root->setAttribute("rotation", mRotation);
    root->setAttribute("width", mWidth);
    root->setAttribute("height", mHeight);
    root->setAttribute("tht_drill", mDrillDiameter);
    if (mLayerId == BoardLayer::BottomCopper)
        root->setAttribute("smt_side", QString("bottom"));
    else
        root->setAttribute("smt_side", QString("top"));
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool FootprintPad::checkAttributesValidity() const noexcept
{
    if (mPadUuid.isNull())                      return false;
    if (typeToString(mType).isEmpty())          return false;
    if (mWidth <= 0)                            return false;
    if (mHeight <= 0)                           return false;
    if (mDrillDiameter < 0)                     return false;
    if ((mType == Type_t::SmtRect) &&
        (mLayerId != BoardLayer::TopCopper) &&
        (mLayerId != BoardLayer::BottomCopper)) return false;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

FootprintPad::Type_t FootprintPad::stringToType(const QString& type) throw (Exception)
{
    if      (type == QLatin1String("tht_rect"))     return Type_t::ThtRect;
    else if (type == QLatin1String("tht_octagon"))  return Type_t::ThtOctagon;
    else if (type == QLatin1String("tht_round"))    return Type_t::ThtRound;
    else if (type == QLatin1String("smt_rect"))     return Type_t::SmtRect;
    else throw RuntimeError(__FILE__, __LINE__, type, type);
}

QString FootprintPad::typeToString(Type_t type) noexcept
{
    switch (type)
    {
        case Type_t::ThtRect:       return QString("tht_rect");
        case Type_t::ThtOctagon:    return QString("tht_octagon");
        case Type_t::ThtRound:      return QString("tht_round");
        case Type_t::SmtRect:       return QString("smt_rect");
        default: Q_ASSERT(false); return QString();
    }
}


/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
