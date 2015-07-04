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

#ifndef PROJECT_BGI_FOOTPRINTPAD_H
#define PROJECT_BGI_FOOTPRINTPAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "bgi_base.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class BoardLayer;

namespace project {
class BI_FootprintPad;
}

namespace library {
class FootprintPad;
}

/*****************************************************************************************
 *  Class BGI_FootprintPad
 ****************************************************************************************/

namespace project {

/**
 * @brief The BGI_FootprintPad class
 *
 * @author ubruhin
 * @date 2015-06-07
 */
class BGI_FootprintPad final : public BGI_Base
{
    public:

        // Constructors / Destructor
        explicit BGI_FootprintPad(BI_FootprintPad& pad) noexcept;
        ~BGI_FootprintPad() noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    private:

        // make some methods inaccessible...
        BGI_FootprintPad() = delete;
        BGI_FootprintPad(const BGI_FootprintPad& other) = delete;
        BGI_FootprintPad& operator=(const BGI_FootprintPad& rhs) = delete;

        // Private Methods
        BoardLayer* getBoardLayer(uint id) const noexcept;


        // General Attributes
        BI_FootprintPad& mPad;
        const library::FootprintPad& mLibPad;
        QFont mFont;

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

#endif // PROJECT_BGI_FOOTPRINTPAD_H
