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
#include <QtWidgets>
#include "boardlayer.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayer::BoardLayer(uint id) :
    QObject(0), mId(id), mName(), mColor(), mColorHighlighted(), mIsVisible(false)
{
    switch (mId)
    {
        case Grid:
            mName = tr("Grid");
            mColor = Qt::white;                 // background
            mColorHighlighted = Qt::lightGray;  // lines
            mIsVisible = true;
            break;

        case OriginCrosses:
            mName = tr("Origin Crosses");
            mColor = QColor(0, 0, 0, 50);
            mColorHighlighted = QColor(0, 0, 0, 80);
            mIsVisible = true;
            break;

        case Unrouted:
            mName = tr("Unrouted");
            mColor = Qt::darkYellow;
            mColorHighlighted = Qt::yellow;
            mIsVisible = true;
            break;

        case FootprintGrabAreas:
            mName = tr("Footprint Grab Areas");
            mColor = QColor(255, 255, 0, 30);
            mColorHighlighted = QColor(255, 255, 0, 50);
            mIsVisible = true;
            break;

        case BoardOutline:
            mName = tr("Board Outline");
            mColor = Qt::lightGray;
            mColorHighlighted = Qt::white;
            mIsVisible = true;
            break;

        case Drills:
            mName = tr("Drills");
            mColor = Qt::lightGray;
            mColorHighlighted = Qt::white;
            mIsVisible = true;
            break;

        case Vias:
            mName = tr("Vias");
            mColor = Qt::darkGreen;
            mColorHighlighted = Qt::green;
            mIsVisible = true;
            break;

        case TopDeviceOutlines:
            mName = tr("TopDeviceOutlines");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case TopOverlayNames:
            mName = tr("TopOverlayNames");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case TopOverlayValues:
            mName = tr("TopOverlayValues");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case TopOverlay:
            mName = tr("TopOverlay");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case TopCopper:
            mName = tr("TopCopper");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            mIsVisible = true;
            break;

        case BottomDeviceOutlines:
            mName = tr("BottomDeviceOutlines");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case BottomOverlayNames:
            mName = tr("BottomOverlayNames");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case BottomOverlayValues:
            mName = tr("BottomOverlayValues");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case BottomOverlay:
            mName = tr("BottomOverlay");
            mColor = Qt::gray;
            mColorHighlighted = Qt::lightGray;
            mIsVisible = true;
            break;

        case BottomCopper:
            mName = tr("BottomCopper");
            mColor = Qt::darkBlue;
            mColorHighlighted = Qt::blue;
            mIsVisible = true;
            break;

#ifdef QT_DEBUG
        case DEBUG_GraphicsItemsBoundingRect:
            mName = tr("DEBUG_GraphicsItemsBoundingRect");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            mIsVisible = false;
            break;

        case DEBUG_GraphicsItemsTextsBoundingRect:
            mName = tr("DEBUG_GraphicsItemsTextsBoundingRect");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            mIsVisible = false;
            break;
#endif

        default:
            mName = tr("TODO");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            mIsVisible = true;
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
