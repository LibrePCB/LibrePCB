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

#ifndef LIBREPCB_PROJECT_BGI_POLYGON_H
#define LIBREPCB_PROJECT_BGI_POLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "bgi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;
class BoardLayer;

namespace project {

class BI_Polygon;

/*****************************************************************************************
 *  Class BGI_Polygon
 ****************************************************************************************/

/**
 * @brief The BGI_Polygon class
 *
 * @author ubruhin
 * @date 2016-01-12
 */
class BGI_Polygon final : public BGI_Base
{
    public:

        // Constructors / Destructor
        explicit BGI_Polygon(BI_Polygon& polygon) noexcept;
        ~BGI_Polygon() noexcept;

        // Getters
        bool isSelectable() const noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    private:

        // make some methods inaccessible...
        BGI_Polygon() = delete;
        BGI_Polygon(const BGI_Polygon& other) = delete;
        BGI_Polygon& operator=(const BGI_Polygon& rhs) = delete;

        // Private Methods
        BoardLayer* getBoardLayer(int id) const noexcept;

        // General Attributes
        BI_Polygon& mBiPolygon;
        const Polygon& mPolygon;

        // Cached Attributes
        BoardLayer* mLayer;
        QRectF mBoundingRect;
        QPainterPath mShape;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BGI_POLYGON_H
