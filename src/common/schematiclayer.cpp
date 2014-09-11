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
#include "schematiclayer.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicLayer::SchematicLayer(unsigned int id) :
    QObject(0), mId(id)
{
    switch (mId)
    {
        case OriginCrosses:
            mName = tr("Origin Crosses");
            mColor = Qt::lightGray;
            mColorHighlighted = Qt::lightGray;
            mFillColor = Qt::lightGray;
            mFillColorHighlighted = Qt::lightGray;
            break;

        case SymbolOutlines:
            mName = tr("Symbol Outlines");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            mFillColor = QColor(255, 255, 0, 50);
            mFillColorHighlighted = QColor(255, 255, 0, 80);
            break;

        case SymbolPinCircles:
            mName = tr("Symbol Pin Circles");
            mColor = Qt::green;
            mColorHighlighted = Qt::green;
            mFillColor = Qt::transparent;
            mFillColorHighlighted = Qt::transparent;
            break;

        case SymbolPinNames:
            mName = tr("Symbol Pin Names");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mFillColor = Qt::gray;
            mFillColorHighlighted = Qt::lightGray;
            break;

        case ComponentNames:
            mName = tr("Component Names");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mFillColor = Qt::gray;
            mFillColorHighlighted = Qt::lightGray;
            break;

        case ComponentValues:
            mName = tr("Component Values");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mFillColor = Qt::gray;
            mFillColorHighlighted = Qt::lightGray;
            break;

        case Nets:
            mName = tr("Nets");
            mColor = Qt::darkGreen;
            mColorHighlighted = Qt::green;
            mFillColor = Qt::darkGreen;
            mFillColorHighlighted = Qt::green;
            break;

        case Busses:
            mName = tr("Busses");
            mColor = Qt::darkBlue;
            mColorHighlighted = Qt::blue;
            mFillColor = Qt::darkBlue;
            mFillColorHighlighted = Qt::blue;
            break;

        default:
            if (mId >= UserDefinedBaseId)
            {
                // TODO: this is a user-defined layer...
            }
            else
            {
                qCritical() << "invalid schematic layer id:" << mId;
            }
            break;
    }
}

SchematicLayer::~SchematicLayer()
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const QColor& SchematicLayer::getColor(bool highlighted) const
{
    return highlighted ? mColorHighlighted : mColor;
}

const QColor& SchematicLayer::getFillColor(bool highlighted) const
{
    return highlighted ? mFillColorHighlighted : mFillColor;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
