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
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayer::BoardLayer(int id) :
    QObject(0), mId(id), mName(), mColor(), mColorHighlighted(), mIsVisible(false)
{
    Q_ASSERT(mId >= 0);

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
            mColor = QColor(255, 255, 255, 150);
            mColorHighlighted = QColor(255, 255, 255, 220);
            mIsVisible = true;
            break;

        case Drills:
            mName = tr("Drills");
            mColor = QColor(255, 255, 255, 150);
            mColorHighlighted = QColor(255, 255, 255, 220);
            mIsVisible = true;
            break;

        case Vias:
            mName = tr("Vias");
            mColor = QColor(0, 255, 0, 150);
            mColorHighlighted = QColor(0, 255, 0, 220);
            mIsVisible = true;
            break;

        case ViaRestrict:
            mName = tr("ViaRestrict");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = true;
            break;

        case TopDeviceOutlines:
            mName = tr("TopDeviceOutlines");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopGlue:
            mName = tr("TopGlue");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = true;
            break;

        case TopPaste:
            mName = tr("TopPaste");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = true;
            break;

        case TopOverlayNames:
            mName = tr("TopOverlayNames");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopOverlayValues:
            mName = tr("TopOverlayValues");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopOverlay:
            mName = tr("TopOverlay");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopDeviceKeepout:
            mName = tr("TopDeviceKeepout");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = true;
            break;

        case TopCopperRestrict:
            mName = tr("TopCopperRestrict");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = true;
            break;

        case TopCopper:
            mName = tr("TopCopper");
            mColor = QColor(255, 0, 0, 150);
            mColorHighlighted = QColor(255, 0, 0, 220);
            mIsVisible = true;
            break;

        case BottomDeviceOutlines:
            mName = tr("BottomDeviceOutlines");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomGlue:
            mName = tr("BottomGlue");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = true;
            break;

        case BottomPaste:
            mName = tr("BottomPaste");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = true;
            break;

        case BottomOverlayNames:
            mName = tr("BottomOverlayNames");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomOverlayValues:
            mName = tr("BottomOverlayValues");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomOverlay:
            mName = tr("BottomOverlay");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomDeviceKeepout:
            mName = tr("BottomDeviceKeepout");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = true;
            break;

        case BottomCopperRestrict:
            mName = tr("BottomCopperRestrict");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = true;
            break;

        case BottomCopper:
            mName = tr("BottomCopper");
            mColor = QColor(0, 0, 255, 150);
            mColorHighlighted = QColor(0, 0, 255, 220);
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
            mColor = QColor(255, 0, 0, 150);
            mColorHighlighted = QColor(255, 0, 0, 220);
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
 *  Static Methods
 ****************************************************************************************/

int BoardLayer::getMirroredLayerId(int id) noexcept
{
    if ((id >= _TOP_LAYERS_START) && (id <= _TOP_LAYERS_END))
        return _BOTTOM_LAYERS_START + (_TOP_LAYERS_END - id);
    else if ((id >= _BOTTOM_LAYERS_START) && (id <= _BOTTOM_LAYERS_END))
        return _TOP_LAYERS_END - (id - _BOTTOM_LAYERS_START);
    else
        return id; // Layer cannot be mirrored
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
