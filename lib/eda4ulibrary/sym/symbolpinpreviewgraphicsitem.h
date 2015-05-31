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

#ifndef LIBRARY_SYMBOLPINPREVIEWGRAPHICSITEM_H
#define LIBRARY_SYMBOLPINPREVIEWGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <eda4ucommon/graphics/graphicsitem.h>
#include "../gencmp/gencompsymbvaritem.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace library {
class SymbolPin;
class GenCompSignal;
}

/*****************************************************************************************
 *  Class SymbolPinPreviewGraphicsItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPinPreviewGraphicsItem class
 *
 * @author ubruhin
 * @date 2015-04-21
 */
class SymbolPinPreviewGraphicsItem final : public GraphicsItem
{
    public:

        // Constructors / Destructor
        explicit SymbolPinPreviewGraphicsItem(const SymbolPin& pin,
                                              const GenCompSignal* genCompSignal,
                                              GenCompSymbVarItem::PinDisplayType_t displayType) noexcept;
        ~SymbolPinPreviewGraphicsItem() noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    private:

        // make some methods inaccessible...
        SymbolPinPreviewGraphicsItem() = delete;
        SymbolPinPreviewGraphicsItem(const SymbolPinPreviewGraphicsItem& other) = delete;
        SymbolPinPreviewGraphicsItem& operator=(const SymbolPinPreviewGraphicsItem& rhs) = delete;


        // General Attributes
        const SymbolPin& mPin;
        const GenCompSignal* mGenCompSignal;
        GenCompSymbVarItem::PinDisplayType_t mDisplayType;
        SchematicLayer* mCircleLayer;
        SchematicLayer* mLineLayer;
        SchematicLayer* mTextLayer;
        QFont mFont;
        qreal mRadiusPx;

        // Cached Attributes
        QStaticText mStaticText;
        bool mRotate180;
        int mFlags;
        QRectF mBoundingRect;
        QPointF mTextOrigin;
        QRectF mTextBoundingRect;
        QPainterPath mShape;
};

} // namespace library

#endif // LIBRARY_SYMBOLPINPREVIEWGRAPHICSITEM_H
