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

#ifndef LIBREPCB_ELLIPSEGRAPHICSITEM_H
#define LIBREPCB_ELLIPSEGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "primitiveellipsegraphicsitem.h"
#include "../geometry/ellipse.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

/*****************************************************************************************
 *  Class EllipseGraphicsItem
 ****************************************************************************************/

/**
 * @brief The EllipseGraphicsItem class
 *
 * @author ubruhin
 * @date 2017-05-28
 */
class EllipseGraphicsItem final : public PrimitiveEllipseGraphicsItem, public IF_EllipseObserver
{
    public:

        // Constructors / Destructor
        EllipseGraphicsItem() = delete;
        EllipseGraphicsItem(const EllipseGraphicsItem& other) = delete;
        EllipseGraphicsItem(Ellipse& ellipse, const IF_GraphicsLayerProvider& lp,
                            QGraphicsItem* parent = nullptr) noexcept;
        ~EllipseGraphicsItem() noexcept;

        // Getters
        Ellipse& getEllipse() noexcept {return mEllipse;}

        // Operator Overloadings
        EllipseGraphicsItem& operator=(const EllipseGraphicsItem& rhs) = delete;


    private: // Methods
        void ellipseLayerNameChanged(const QString& newLayerName) noexcept override;
        void ellipseLineWidthChanged(const Length& newLineWidth) noexcept override;
        void ellipseIsFilledChanged(bool newIsFilled) noexcept override;
        void ellipseIsGrabAreaChanged(bool newIsGrabArea) noexcept override;
        void ellipseCenterChanged(const Point& newCenter) noexcept override;
        void ellipseRadiusXChanged(const Length& newRadiusX) noexcept override;
        void ellipseRadiusYChanged(const Length& newRadiusY) noexcept override;
        void ellipseRotationChanged(const Angle& newRotation) noexcept override;
        void updateFillLayer() noexcept;


    private: // Data
        Ellipse& mEllipse;
        const IF_GraphicsLayerProvider& mLayerProvider;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ELLIPSEGRAPHICSITEM_H
