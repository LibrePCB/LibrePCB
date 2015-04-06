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

#ifndef PROJECT_SGI_SYMBOLPIN_H
#define PROJECT_SGI_SYMBOLPIN_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "sgi_base.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace project {
class SI_SymbolPin;
}

namespace library {
class SymbolPin;
}

/*****************************************************************************************
 *  Class SGI_SymbolPin
 ****************************************************************************************/

namespace project {

/**
 * @brief The SGI_SymbolPin class
 *
 * @author ubruhin
 * @date 2014-08-23
 */
class SGI_SymbolPin final : public SGI_Base
{
    public:

        // Types

        /// to make  qgraphicsitem_cast() working
        enum {Type = Schematic::Type_SymbolPin};

        // Constructors / Destructor
        explicit SGI_SymbolPin(SI_SymbolPin& pin) noexcept;
        ~SGI_SymbolPin() noexcept;

        // Getters
        SI_SymbolPin& getPin() const noexcept {return mPin;}

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        int type() const {return Type;} ///< to make  qgraphicsitem_cast() working
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    private:

        // make some methods inaccessible...
        SGI_SymbolPin() = delete;
        SGI_SymbolPin(const SGI_SymbolPin& other) = delete;
        SGI_SymbolPin& operator=(const SGI_SymbolPin& rhs) = delete;

        // Private Methods
        SchematicLayer* getSchematicLayer(uint id) const noexcept;


        // General Attributes
        SI_SymbolPin& mPin;
        const library::SymbolPin& mLibPin;
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

} // namespace project

#endif // PROJECT_SGI_SYMBOLPIN_H
