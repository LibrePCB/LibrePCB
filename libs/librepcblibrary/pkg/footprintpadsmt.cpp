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
#include "footprintpadsmt.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPadSmt::FootprintPadSmt(const Uuid& padUuid, const Point& pos, const Angle& rot,
                                 const Length& width, const Length& height, BoardSide_t side) noexcept :
    FootprintPad(Technology_t::SMT, padUuid, pos, rot, width, height),
    mBoardSide(side)
{
}

FootprintPadSmt::FootprintPadSmt(const XmlDomElement& domElement) throw (Exception) :
    FootprintPad(domElement)
{
    // read attributes
    mBoardSide = stringToBoardSide(domElement.getAttribute<QString>("side", true));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

FootprintPadSmt::~FootprintPadSmt() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

int FootprintPadSmt::getLayerId() const noexcept
{
    switch (mBoardSide)
    {
        case BoardSide_t::TOP:      return BoardLayer::LayerID::TopCopper;
        case BoardSide_t::BOTTOM:   return BoardLayer::LayerID::BottomCopper;
        default: Q_ASSERT(false);   return -1;
    }
}

const QPainterPath& FootprintPadSmt::toQPainterPathPx() const noexcept
{
    if (mPainterPathPx.isEmpty()) {
        mPainterPathPx.setFillRule(Qt::WindingFill);
        mPainterPathPx.addRect(getBoundingRectPx());
    }
    return mPainterPathPx;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

void FootprintPadSmt::setBoardSide(BoardSide_t side) noexcept
{
    mBoardSide = side;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* FootprintPadSmt::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(FootprintPad::serializeToXmlDomElement());
    root->setAttribute("side", boardSideToString(mBoardSide));
    return root.take();
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

FootprintPadSmt::BoardSide_t FootprintPadSmt::stringToBoardSide(const QString& side) throw (Exception)
{
    if      (side == QLatin1String("top"))      return BoardSide_t::TOP;
    else if (side == QLatin1String("bottom"))   return BoardSide_t::BOTTOM;
    else throw RuntimeError(__FILE__, __LINE__, side, side);
}

QString FootprintPadSmt::boardSideToString(BoardSide_t side) noexcept
{
    switch (side)
    {
        case BoardSide_t::TOP:    return QString("top");
        case BoardSide_t::BOTTOM: return QString("bottom");
        default: Q_ASSERT(false); return QString();
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool FootprintPadSmt::checkAttributesValidity() const noexcept
{
    if (!FootprintPad::checkAttributesValidity())   return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
