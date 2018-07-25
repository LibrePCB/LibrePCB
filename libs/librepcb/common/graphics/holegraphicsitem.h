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

#ifndef LIBREPCB_HOLEGRAPHICSITEM_H
#define LIBREPCB_HOLEGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "primitivecirclegraphicsitem.h"
#include "../geometry/hole.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class OriginCrossGraphicsItem;
class IF_GraphicsLayerProvider;

/*****************************************************************************************
 *  Class HoleGraphicsItem
 ****************************************************************************************/

/**
 * @brief The HoleGraphicsItem class is the graphical representation of a librepcb::Text
 *
 * @author ubruhin
 * @date 2017-05-30
 */
class HoleGraphicsItem final : public PrimitiveCircleGraphicsItem, public IF_HoleObserver
{
    public:

        // Constructors / Destructor
        HoleGraphicsItem() = delete;
        HoleGraphicsItem(const HoleGraphicsItem& other) = delete;
        HoleGraphicsItem(Hole& hole, const IF_GraphicsLayerProvider& lp,
                         QGraphicsItem* parent = nullptr) noexcept;
        ~HoleGraphicsItem() noexcept;

        // Getters
        Hole& getHole() noexcept {return mHole;}

        // Inherited from QGraphicsItem
        QPainterPath shape() const noexcept override;

        // Operator Overloadings
        HoleGraphicsItem& operator=(const HoleGraphicsItem& rhs) = delete;


    private: // Methods
        void holePositionChanged(const Point& newPos) noexcept override;
        void holeDiameterChanged(const PositiveLength& newDiameter) noexcept override;
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) noexcept override;


    private: // Data
        Hole& mHole;
        const IF_GraphicsLayerProvider& mLayerProvider;
        QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_HOLEGRAPHICSITEM_H
