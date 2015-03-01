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
#include "alignment.h"

/*****************************************************************************************
 *  Class HAlign
 ****************************************************************************************/

QString HAlign::toString() const noexcept
{
    switch (mAlign)
    {
        case Qt::AlignLeft:         return QString("left");
        case Qt::AlignHCenter:      return QString("center");
        case Qt::AlignRight:        return QString("right");
        default: Q_ASSERT(false);   return QString();
    }
}


HAlign HAlign::fromString(const QString& align) throw (Exception)
{
    if (align == "left")
        return HAlign(Qt::AlignLeft);
    else if (align == "center")
        return HAlign(Qt::AlignHCenter);
    else if (align == "right")
        return HAlign(Qt::AlignRight);
    else
    {
        throw RuntimeError(__FILE__, __LINE__, align,
            QString(tr("Invalid horizontal alignment: \"%1\"")).arg(align));
    }
}

/*****************************************************************************************
 *  Class VAlign
 ****************************************************************************************/

QString VAlign::toString() const noexcept
{
    switch (mAlign)
    {
        case Qt::AlignTop:          return QString("top");
        case Qt::AlignVCenter:      return QString("center");
        case Qt::AlignBottom:       return QString("bottom");
        default: Q_ASSERT(false);   return QString();
    }
}

VAlign VAlign::fromString(const QString& align) throw (Exception)
{
    if (align == "top")
        return VAlign(Qt::AlignTop);
    else if (align == "center")
        return VAlign(Qt::AlignVCenter);
    else if (align == "bottom")
        return VAlign(Qt::AlignBottom);
    else
    {
        throw RuntimeError(__FILE__, __LINE__, align,
            QString(tr("Invalid vertical alignment: \"%1\"")).arg(align));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
