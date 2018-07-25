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

#ifndef LIBREPCB_POLYGONGRAPHICSITEM_H
#define LIBREPCB_POLYGONGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "primitivepathgraphicsitem.h"
#include "../geometry/polygon.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

/*****************************************************************************************
 *  Class PolygonGraphicsItem
 ****************************************************************************************/

/**
 * @brief The PolygonGraphicsItem class
 *
 * @author ubruhin
 * @date 2017-05-28
 */
class PolygonGraphicsItem final : public PrimitivePathGraphicsItem, public IF_PolygonObserver
{
    public:

        // Constructors / Destructor
        PolygonGraphicsItem() = delete;
        PolygonGraphicsItem(const PolygonGraphicsItem& other) = delete;
        PolygonGraphicsItem(Polygon& polygon, const IF_GraphicsLayerProvider& lp,
                            QGraphicsItem* parent = nullptr) noexcept;
        ~PolygonGraphicsItem() noexcept;

        // Getters
        Polygon& getPolygon() noexcept {return mPolygon;}

        // Operator Overloadings
        PolygonGraphicsItem& operator=(const PolygonGraphicsItem& rhs) = delete;


    private: // Methods
        void polygonLayerNameChanged(const QString& newLayerName) noexcept override;
        void polygonLineWidthChanged(const UnsignedLength& newLineWidth) noexcept override;
        void polygonIsFilledChanged(bool newIsFilled) noexcept override;
        void polygonIsGrabAreaChanged(bool newIsGrabArea) noexcept override;
        void polygonPathChanged(const Path& newPath) noexcept override;
        void updateFillLayer() noexcept;


    private: // Data
        Polygon& mPolygon;
        const IF_GraphicsLayerProvider& mLayerProvider;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_POLYGONGRAPHICSITEM_H
