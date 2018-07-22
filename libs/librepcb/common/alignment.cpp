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
#include "alignment.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class HAlign
 ****************************************************************************************/

HAlign& HAlign::mirror() noexcept
{
    switch (mAlign)
    {
        case Qt::AlignLeft:     mAlign = Qt::AlignRight;    break;
        case Qt::AlignRight:    mAlign = Qt::AlignLeft;     break;
        case Qt::AlignHCenter:  break;
        default: Q_ASSERT(false); break;
    }
    return *this;
}

/*****************************************************************************************
 *  Class VAlign
 ****************************************************************************************/

VAlign& VAlign::mirror() noexcept
{
    switch (mAlign)
    {
        case Qt::AlignTop:      mAlign = Qt::AlignBottom;   break;
        case Qt::AlignBottom:   mAlign = Qt::AlignTop;      break;
        case Qt::AlignVCenter:  break;
        default: Q_ASSERT(false); break;
    }
    return *this;
}

/*****************************************************************************************
 *  Class VAlign
 ****************************************************************************************/

Alignment::Alignment(const SExpression& node)
{
    try {
        mH = node.getChildByIndex(0).getValue<HAlign>();
        mV = node.getChildByIndex(1).getValue<VAlign>();
    } catch (const Exception& e) {
        throw FileParseError(__FILE__, __LINE__, node.getFilePath(), -1, -1,
                             QString(), e.getMsg());
    }
}

Alignment& Alignment::mirror() noexcept
{
    mH.mirror();
    mV.mirror();
    return *this;
}

Alignment& Alignment::mirrorH() noexcept
{
    mH.mirror();
    return *this;
}

Alignment& Alignment::mirrorV() noexcept
{
    mV.mirror();
    return *this;
}

void Alignment::serialize(SExpression& root) const
{
    root.appendChild(mH);
    root.appendChild(mV);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
