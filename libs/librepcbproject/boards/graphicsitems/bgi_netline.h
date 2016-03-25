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

#ifndef LIBREPCB_PROJECT_BGI_NETLINE_H
#define LIBREPCB_PROJECT_BGI_NETLINE_H

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

class BoardLayer;

namespace project {

class BI_NetLine;

/*****************************************************************************************
 *  Class BGI_NetLine
 ****************************************************************************************/

/**
 * @brief The BGI_NetLine class
 */
class BGI_NetLine final : public BGI_Base
{
    public:

        // Constructors / Destructor
        explicit BGI_NetLine(BI_NetLine& netline) noexcept;
        ~BGI_NetLine() noexcept;

        // Getters
        bool isSelectable() const noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const {return mBoundingRect;}
        QPainterPath shape() const {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);


    private:

        // make some methods inaccessible...
        BGI_NetLine() = delete;
        BGI_NetLine(const BGI_NetLine& other) = delete;
        BGI_NetLine& operator=(const BGI_NetLine& rhs) = delete;

        // Private Methods
        BoardLayer* getBoardLayer(int id) const noexcept;

        // Attributes
        BI_NetLine& mNetLine;
        BoardLayer* mLayer;

        // Cached Attributes
        QLineF mLineF;
        QRectF mBoundingRect;
        QPainterPath mShape;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BGI_NETLINE_H
