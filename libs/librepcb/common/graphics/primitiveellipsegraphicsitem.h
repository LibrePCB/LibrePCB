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

#ifndef LIBREPCB_PRIMITIVEELLIPSEGRAPHICSITEM_H
#define LIBREPCB_PRIMITIVEELLIPSEGRAPHICSITEM_H

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
 *  Class PrimitiveEllipseGraphicsItem
 ****************************************************************************************/

/**
 * @brief The PrimitiveEllipseGraphicsItem class
 *
 * @author ubruhin
 * @date 2017-05-28
 */
class PrimitiveEllipseGraphicsItem : public QGraphicsItem, public IF_GraphicsLayerObserver
{
    public:

        // Constructors / Destructor
        //PrimitiveEllipseGraphicsItem() = delete;
        PrimitiveEllipseGraphicsItem(const PrimitiveEllipseGraphicsItem& other) = delete;
        explicit PrimitiveEllipseGraphicsItem(QGraphicsItem* parent = nullptr) noexcept;
        virtual ~PrimitiveEllipseGraphicsItem() noexcept;

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;
        void setRadiusX(const Length& rx) noexcept;
        void setRadiusY(const Length& ry) noexcept;
        void setRadius(const Length& rx, const Length& ry) noexcept;
        void setLineWidth(const Length& width) noexcept;
        void setLineLayer(const GraphicsLayer* layer) noexcept;
        void setFillLayer(const GraphicsLayer* layer) noexcept;

        // Inherited from IF_LayerObserver
        void layerColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept override;
        void layerHighlightColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept override;
        void layerVisibleChanged(const GraphicsLayer& layer, bool newVisible) noexcept override;
        void layerEnabledChanged(const GraphicsLayer& layer, bool newEnabled) noexcept override;
        void layerDestroyed(const GraphicsLayer& layer) noexcept override;

        // Inherited from QGraphicsItem
        virtual QRectF boundingRect() const noexcept override {return mBoundingRect;}
        virtual QPainterPath shape() const noexcept override {return mShape;}
        virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) noexcept override;

        // Operator Overloadings
        PrimitiveEllipseGraphicsItem& operator=(const PrimitiveEllipseGraphicsItem& rhs) = delete;


    private: // Methods
        void updateColors() noexcept;
        void updateBoundingRectAndShape() noexcept;
        void updateVisibility() noexcept;


    private: // Data
        const GraphicsLayer* mLineLayer;
        const GraphicsLayer* mFillLayer;
        QPen mPen;
        QPen mPenHighlighted;
        QBrush mBrush;
        QBrush mBrushHighlighted;
        QRectF mEllipseRect;
        QRectF mBoundingRect;
        QPainterPath mShape;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_PRIMITIVEELLIPSEGRAPHICSITEM_H
