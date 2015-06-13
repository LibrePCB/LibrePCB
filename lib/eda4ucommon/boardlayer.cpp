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
#include <QtWidgets>
#include "boardlayer.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayer::BoardLayer(uint id) :
    QObject(0), mId(id)
{
    switch (mId)
    {
        case Grid:
            mName = tr("Grid");
            mColor = Qt::white;                 // background
            mColorHighlighted = Qt::lightGray;  // lines
            break;

        case OriginCrosses:
            mName = tr("Origin Crosses");
            mColor = QColor(0, 0, 0, 50);
            mColorHighlighted = QColor(0, 0, 0, 80);
            break;

        case Unrouted:
            mName = tr("Unrouted");
            mColor = Qt::darkYellow;
            mColorHighlighted = Qt::yellow;
            break;

        case BoardOutline:
            mName = tr("Board Outline");
            mColor = Qt::lightGray;
            mColorHighlighted = Qt::white;
            break;

        default:
            mName = tr("TODO");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            break;

        /*default:
            if (mId >= UserDefinedBaseId)
            {
                // TODO: this is a user-defined layer...
            }
            else
            {
                qCritical() << "invalid schematic layer id:" << mId;
            }
            break;*/
    }
}

BoardLayer::~BoardLayer()
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const QColor& BoardLayer::getColor(bool highlighted) const
{
    return highlighted ? mColorHighlighted : mColor;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
