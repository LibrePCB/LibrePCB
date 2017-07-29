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
#include <QtWidgets>
#include "boardlayer.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayer::BoardLayer(const BoardLayer& other) throw (Exception) :
    QObject(0), mId(other.mId), mName(other.mName), mColor(other.mColor),
    mColorHighlighted(other.mColorHighlighted), mIsVisible(other.mIsVisible)
{
}

BoardLayer::BoardLayer(const DomElement& domElement) throw (Exception) :
    QObject(0), mId(-1), mName(), mColor(), mColorHighlighted(), mIsVisible(false)
{
    mId = domElement.getAttribute<uint>("id", true);
    mName = domElement.getText<QString>(true);
    mColor = domElement.getAttribute<QColor>("color", true);
    mColorHighlighted = domElement.getAttribute<QColor>("color_hl", true);
    mIsVisible = domElement.getAttribute<bool>("visible", true);
}

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

        case Unrouted:
            mName = tr("Unrouted");
            mColor = Qt::darkYellow;
            mColorHighlighted = Qt::yellow;
            mIsVisible = true;
            break;

        case BoardOutlines:
            mName = tr("Board Outlines");
            mColor = QColor(255, 255, 255, 180);
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
            mName = tr("Via Restrict");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = false;
            break;

        case ThtPads:
            mName = tr("THT Pads");
            mColor = QColor(0, 255, 0, 150);
            mColorHighlighted = QColor(0, 255, 0, 220);
            mIsVisible = true;
            break;

        case TopDeviceOutlines:
            mName = tr("Top Device Outlines");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopDeviceOriginCrosses:
            mName = tr("Top Device Origin Crosses");
            mColor = QColor(255, 255, 255, 50);
            mColorHighlighted = QColor(255, 255, 255, 80);
            mIsVisible = true;
            break;

        case TopDeviceGrabAreas:
            mName = tr("Top Device Grab Areas");
            mColor = QColor(255, 255, 255, 20);
            mColorHighlighted = QColor(255, 255, 255, 50);
            mIsVisible = false;
            break;

        case TopTestPoints:
            mName = tr("Top Test Points");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopGlue:
            mName = tr("Top Glue");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = false;
            break;

        case TopPaste:
            mName = tr("Top Paste");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = false;
            break;

        case TopOverlayNames:
            mName = tr("Top Overlay Names");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopOverlayValues:
            mName = tr("Top Overlay Values");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopOverlay:
            mName = tr("Top Overlay");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case TopStopMask:
            mName = tr("Top Stop Mask");
            mColor = QColor(255, 255, 255, 100);
            mColorHighlighted = QColor(255, 0, 0, 150);
            mIsVisible = false;
            break;

        case TopDeviceKeepout:
            mName = tr("Top Device Keepout");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = false;
            break;

        case TopCopperRestrict:
            mName = tr("Top Copper Restrict");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = false;
            break;

        case TopCopper:
            mName = tr("Top Copper");
            mColor = QColor(255, 0, 0, 130);
            mColorHighlighted = QColor(255, 0, 0, 220);
            mIsVisible = true;
            break;

        case BottomDeviceOriginCrosses:
            mName = tr("Bottom Device Origin Crosses");
            mColor = QColor(255, 255, 255, 50);
            mColorHighlighted = QColor(255, 255, 255, 80);
            mIsVisible = true;
            break;

        case BottomDeviceGrabAreas:
            mName = tr("Bottom Device Grab Areas");
            mColor = QColor(255, 255, 255, 20);
            mColorHighlighted = QColor(255, 255, 255, 50);
            mIsVisible = false;
            break;

        case BottomTestPoints:
            mName = tr("Bottom Test Points");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomDeviceOutlines:
            mName = tr("Bottom Device Outlines");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomGlue:
            mName = tr("Bottom Glue");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = false;
            break;

        case BottomPaste:
            mName = tr("Bottom Paste");
            mColor = QColor(224, 224, 224, 100);
            mColorHighlighted = QColor(224, 224, 224, 120);
            mIsVisible = false;
            break;

        case BottomOverlayNames:
            mName = tr("Bottom Overlay Names");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomOverlayValues:
            mName = tr("Bottom Overlay Values");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomOverlay:
            mName = tr("Bottom Overlay");
            mColor = QColor(224, 224, 224, 150);
            mColorHighlighted = QColor(224, 224, 224, 220);
            mIsVisible = true;
            break;

        case BottomStopMask:
            mName = tr("Bottom Stop Mask");
            mColor = QColor(255, 255, 255, 100);
            mColorHighlighted = QColor(255, 0, 0, 150);
            mIsVisible = false;
            break;

        case BottomDeviceKeepout:
            mName = tr("Bottom Device Keepout");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = false;
            break;

        case BottomCopperRestrict:
            mName = tr("Bottom Copper Restrict");
            mColor = QColor(255, 255, 0, 50);
            mColorHighlighted = QColor(255, 255, 0, 80);
            mIsVisible = false;
            break;

        case BottomCopper:
            mName = tr("Bottom Copper");
            mColor = QColor(0, 0, 255, 130);
            mColorHighlighted = QColor(0, 0, 255, 220);
            mIsVisible = true;
            break;

#ifdef QT_DEBUG
        case DEBUG_GraphicsItemsBoundingRects:
            mName = tr("[DEBUG] GraphicsItems Bounding Rects");
            mColor = Qt::darkRed;
            mColorHighlighted = Qt::red;
            mIsVisible = false;
            break;

        case DEBUG_GraphicsItemsTextsBoundingRects:
            mName = tr("[DEBUG] GraphicsItems Texts Bounding Rects");
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
 *  General Methods
 ****************************************************************************************/

void BoardLayer::serialize(DomElement& root) const throw (Exception)
{
    root.setAttribute("id", mId);
    root.setText(mName);
    root.setAttribute("color", mColor);
    root.setAttribute("color_hl", mColorHighlighted);
    root.setAttribute("visible", mIsVisible);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

bool BoardLayer::isCopperLayer(int id) noexcept
{
    return ((id >= _COPPER_LAYERS_START) && (id <= _COPPER_LAYERS_END));
}

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
