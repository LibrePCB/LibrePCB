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

#ifndef LIBREPCB_CIRCLEGRAPHICSITEM_H
#define LIBREPCB_CIRCLEGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "primitivecirclegraphicsitem.h"
#include "../geometry/circle.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

/*****************************************************************************************
 *  Class CircleGraphicsItem
 ****************************************************************************************/

/**
 * @brief The CircleGraphicsItem class
 *
 * @author ubruhin
 * @date 2017-05-28
 */
class CircleGraphicsItem final : public PrimitiveCircleGraphicsItem, public IF_CircleObserver
{
    public:

        // Constructors / Destructor
        CircleGraphicsItem() = delete;
        CircleGraphicsItem(const CircleGraphicsItem& other) = delete;
        CircleGraphicsItem(Circle& circle, const IF_GraphicsLayerProvider& lp,
                            QGraphicsItem* parent = nullptr) noexcept;
        ~CircleGraphicsItem() noexcept;

        // Getters
        Circle& getCircle() noexcept {return mCircle;}

        // Operator Overloadings
        CircleGraphicsItem& operator=(const CircleGraphicsItem& rhs) = delete;


    private: // Methods
        void circleLayerNameChanged(const GraphicsLayerName& newLayerName) noexcept override;
        void circleLineWidthChanged(const UnsignedLength& newLineWidth) noexcept override;
        void circleIsFilledChanged(bool newIsFilled) noexcept override;
        void circleIsGrabAreaChanged(bool newIsGrabArea) noexcept override;
        void circleCenterChanged(const Point& newCenter) noexcept override;
        void circleDiameterChanged(const PositiveLength& newDiameter) noexcept override;
        void updateFillLayer() noexcept;


    private: // Data
        Circle& mCircle;
        const IF_GraphicsLayerProvider& mLayerProvider;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CIRCLEGRAPHICSITEM_H
