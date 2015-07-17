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

#ifndef PROJECT_BGI_FOOTPRINT_H
#define PROJECT_BGI_FOOTPRINT_H

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
class BI_Footprint;
}

namespace library {
class Footprint;
class FootprintText;
}

/*****************************************************************************************
 *  Class BGI_Footprint
 ****************************************************************************************/

namespace project {

/**
 * @brief The BGI_Footprint class
 *
 * @author ubruhin
 * @date 2015-05-24
 */
class BGI_Footprint final : public BGI_Base
{
    public:

        // Constructors / Destructor
        explicit BGI_Footprint(BI_Footprint& footprint) noexcept;
        ~BGI_Footprint() noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    private:

        // make some methods inaccessible...
        BGI_Footprint() = delete;
        BGI_Footprint(const BGI_Footprint& other) = delete;
        BGI_Footprint& operator=(const BGI_Footprint& rhs) = delete;

        // Private Methods
        BoardLayer* getBoardLayer(int id) const noexcept;


        // Types

        struct CachedTextProperties_t {
            QString text;
            int fontPixelSize;
            qreal scaleFactor;
            bool rotate180;
            int flags;
            QRectF textRect;    // not scaled
        };


        // General Attributes
        BI_Footprint& mFootprint;
        const library::Footprint& mLibFootprint;
        QFont mFont;

        // Cached Attributes
        QRectF mBoundingRect;
        QPainterPath mShape;
        QHash<const library::FootprintText*, CachedTextProperties_t> mCachedTextProperties;
};

} // namespace project

#endif // PROJECT_BGI_FOOTPRINT_H
