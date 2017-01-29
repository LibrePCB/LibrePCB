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

#ifndef LIBREPCB_ORIGINCROSSGRAPHICSITEM_H
#define LIBREPCB_ORIGINCROSSGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "../graphics/graphicslayer.h"
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class OriginCrossGraphicsItem
 ****************************************************************************************/

/**
 * @brief The OriginCrossGraphicsItem class
 *
 * @author ubruhin
 * @date 2016-11-13
 */
class OriginCrossGraphicsItem final : public QGraphicsItem, public IF_GraphicsLayerObserver
{
    public:

        // Constructors / Destructor
        //OriginCrossGraphicsItem() = delete;
        OriginCrossGraphicsItem(const OriginCrossGraphicsItem& other) = delete;
        explicit OriginCrossGraphicsItem(QGraphicsItem* parent = nullptr) noexcept;
        ~OriginCrossGraphicsItem() noexcept;

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;
        void setSize(const Length& size) noexcept;
        void setLayer(const GraphicsLayer* layer) noexcept;

        // Inherited from IF_LayerObserver
        void layerColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept override;
        void layerHighlightColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept override;
        void layerVisibleChanged(const GraphicsLayer& layer, bool newVisible) noexcept override;
        void layerEnabledChanged(const GraphicsLayer& layer, bool newEnabled) noexcept override;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept override {return mBoundingRect;}
        QPainterPath shape() const noexcept override {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) noexcept override;

        // Operator Overloadings
        OriginCrossGraphicsItem& operator=(const OriginCrossGraphicsItem& rhs) = delete;


    private: // Methods
        void updateBoundingRectAndShape() noexcept;


    private: // Data
        const GraphicsLayer* mLayer;
        QPen mPen;
        QPen mPenHighlighted;
        Length mSize;
        QLineF mLineH;
        QLineF mLineV;
        QRectF mBoundingRect;
        QPainterPath mShape;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ORIGINCROSSGRAPHICSITEM_H
