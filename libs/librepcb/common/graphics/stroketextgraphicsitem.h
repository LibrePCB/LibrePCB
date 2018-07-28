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

#ifndef LIBREPCB_STROKETEXTGRAPHICSITEM_H
#define LIBREPCB_STROKETEXTGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "primitivepathgraphicsitem.h"
#include "../geometry/stroketext.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class OriginCrossGraphicsItem;
class IF_GraphicsLayerProvider;

/*****************************************************************************************
 *  Class StrokeTextGraphicsItem
 ****************************************************************************************/

/**
 * @brief The StrokeTextGraphicsItem class is the graphical representation of a
 *        librepcb::StrokeText
 */
class StrokeTextGraphicsItem final : public PrimitivePathGraphicsItem,
                                     public IF_StrokeTextObserver
{
    public:

        // Constructors / Destructor
        StrokeTextGraphicsItem() = delete;
        StrokeTextGraphicsItem(const StrokeTextGraphicsItem& other) = delete;
        StrokeTextGraphicsItem(StrokeText& text, const IF_GraphicsLayerProvider& lp,
                               QGraphicsItem* parent = nullptr) noexcept;
        ~StrokeTextGraphicsItem() noexcept;

        // Getters
        StrokeText& getText() noexcept {return mText;}

        // Inherited from QGraphicsItem
        QPainterPath shape() const noexcept override;

        // Operator Overloadings
        StrokeTextGraphicsItem& operator=(const StrokeTextGraphicsItem& rhs) = delete;


    private: // Methods
        void strokeTextLayerNameChanged(const GraphicsLayerName& newLayerName) noexcept override;
        void strokeTextTextChanged(const QString& newText) noexcept override;
        void strokeTextPositionChanged(const Point& newPos) noexcept override;
        void strokeTextRotationChanged(const Angle& newRot) noexcept override;
        void strokeTextHeightChanged(const PositiveLength& newHeight) noexcept override;
        void strokeTextStrokeWidthChanged(const UnsignedLength& newStrokeWidth) noexcept override;
        void strokeTextLetterSpacingChanged(const StrokeTextSpacing& spacing) noexcept override;
        void strokeTextLineSpacingChanged(const StrokeTextSpacing& spacing) noexcept override;
        void strokeTextAlignChanged(const Alignment& newAlign) noexcept override;
        void strokeTextMirroredChanged(bool mirrored) noexcept override;
        void strokeTextAutoRotateChanged(bool newAutoRotate) noexcept override;
        void strokeTextPathsChanged(const QVector<Path>& paths) noexcept override;
        void updateLayer(const GraphicsLayerName& layerName) noexcept;
        void updateTransform() noexcept;
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) noexcept override;


    private: // Data
        StrokeText& mText;
        const IF_GraphicsLayerProvider& mLayerProvider;
        QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_STROKETEXTGRAPHICSITEM_H
