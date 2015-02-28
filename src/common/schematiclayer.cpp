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

SchematicLayer::SchematicLayer(uint id) :
    QObject(0), mId(id)
{
    switch (mId)
    {
        case OriginCrosses:
            mName = tr("Origin Crosses");
            mColor = QColor(0, 0, 0, 50);
            mColorHighlighted = QColor(0, 0, 0, 80);
            break;

        case SymbolOutlines:
            mName = tr("Symbol Outlines");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            break;

        case SymbolGrabAreas:
            mName = tr("Symbol Grab Areas");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            break;

        case SymbolPinCircles:
            mName = tr("Symbol Pin Circles");
            mColor = Qt::green;
            mColorHighlighted = Qt::green;
            break;

        case SymbolPinNames:
            mName = tr("Symbol Pin Names");
            mColor = Qt::darkGray;
            mColorHighlighted = Qt::gray;
            break;

        case ComponentNames:
            mName = tr("Component Names");
            mColor = Qt::darkGray;
            mColorHighlighted = Qt::gray;
            break;

        case ComponentValues:
            mName = tr("Component Values");
            mColor = Qt::darkGray;
            mColorHighlighted = Qt::gray;
            break;

        case Nets:
            mName = tr("Nets");
            mColor = Qt::darkGreen;
            mColorHighlighted = Qt::green;
            break;

        case Busses:
            mName = tr("Busses");
            mColor = Qt::darkBlue;
            mColorHighlighted = Qt::blue;
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

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

QList<SchematicLayer::LayerID> SchematicLayer::getAllLayerIDs() noexcept
{
    QList<LayerID> IDs;
    IDs << SchematicLayer::OriginCrosses    << SchematicLayer::SymbolOutlines
        << SchematicLayer::SymbolPinCircles << SchematicLayer::SymbolPinNames
        << SchematicLayer::ComponentNames   << SchematicLayer::ComponentValues
        << SchematicLayer::Nets             << SchematicLayer::Busses
        << SchematicLayer::SymbolGrabAreas;
    return IDs;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
