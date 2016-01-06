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
#include "footprintpadsmt.h"
#include "footprintpadtht.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPad::FootprintPad(Technology_t technology, const Uuid& padUuid,
                           const Point& pos, const Angle& rot, const Length& width,
                           const Length& height) noexcept :
    mTechnology(technology), mUuid(padUuid), mPosition(pos), mRotation(rot),
    mWidth(width), mHeight(height)
{
}

FootprintPad::FootprintPad(const XmlDomElement& domElement) throw (Exception)
{
    // read attributes
    mTechnology = stringToTechnology(domElement.getAttribute<QString>("technology", true));
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    mRotation = domElement.getAttribute<Angle>("rotation", true);
    mWidth = domElement.getAttribute<Length>("width", true);
    mHeight = domElement.getAttribute<Length>("height", true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

FootprintPad::~FootprintPad() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QRectF FootprintPad::getBoundingRectPx() const noexcept
{
    return QRectF(-mWidth.toPx()/2, -mHeight.toPx()/2, mWidth.toPx(), mHeight.toPx());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/
void FootprintPad::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
}

void FootprintPad::setRotation(const Angle& rot) noexcept
{
    mRotation = rot;
}

void FootprintPad::setWidth(const Length& width) noexcept
{
    Q_ASSERT(width > 0);
    mWidth = width;
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

void FootprintPad::setHeight(const Length& height) noexcept
{
    Q_ASSERT(height > 0);
    mHeight = height;
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* FootprintPad::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("pad"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("technology", technologyToString(mTechnology));
    root->setAttribute("x", mPosition.getX());
    root->setAttribute("y", mPosition.getY());
    root->setAttribute("rotation", mRotation);
    root->setAttribute("width", mWidth);
    root->setAttribute("height", mHeight);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool FootprintPad::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                             return false;
    if (technologyToString(mTechnology).isEmpty())  return false;
    if (mWidth <= 0)                                return false;
    if (mHeight <= 0)                               return false;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

FootprintPad::Technology_t FootprintPad::stringToTechnology(const QString& technology) throw (Exception)
{
    if      (technology == QLatin1String("tht")) return Technology_t::THT;
    else if (technology == QLatin1String("smt")) return Technology_t::SMT;
    else throw RuntimeError(__FILE__, __LINE__, QString(), technology);
}

QString FootprintPad::technologyToString(Technology_t technology) noexcept
{
    switch (technology)
    {
        case Technology_t::THT: return QString("tht");
        case Technology_t::SMT: return QString("smt");
        default: Q_ASSERT(false); return QString();
    }
}

FootprintPad* FootprintPad::fromDomElement(const XmlDomElement& domElement) throw (Exception)
{
    Technology_t technology = stringToTechnology(domElement.getAttribute<QString>("technology", true));
    switch (technology)
    {
        case Technology_t::THT: return new FootprintPadTht(domElement);
        case Technology_t::SMT: return new FootprintPadSmt(domElement);
        default: Q_ASSERT(false); throw LogicError(__FILE__, __LINE__);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
