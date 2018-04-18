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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "stroketextgraphicsitem.h"
#include "origincrossgraphicsitem.h"
#include "../graphics/graphicslayer.h"
#include "../font/strokefontpool.h"
#include "../application.h"
#include "../toolbox.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

StrokeTextGraphicsItem::StrokeTextGraphicsItem(StrokeText& text,
        const IF_GraphicsLayerProvider& lp, QGraphicsItem* parent) noexcept :
    PrimitivePathGraphicsItem(parent), mText(text), mLayerProvider(lp)
{
    // add origin cross
    mOriginCrossGraphicsItem.reset(new OriginCrossGraphicsItem(this));
    mOriginCrossGraphicsItem->setSize(Length(1000000));

    // set text properties
    setPosition(mText.getPosition());
    setLineWidth(mText.getStrokeWidth());
    setPath(Path::toQPainterPathPx(mText.getPaths()));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(5);
    updateLayer(mText.getLayerName());
    updateTransform();

    // register to the text to get attribute updates
    mText.registerObserver(*this);
}

StrokeTextGraphicsItem::~StrokeTextGraphicsItem() noexcept
{
    mText.unregisterObserver(*this);
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

QPainterPath StrokeTextGraphicsItem::shape() const noexcept
{
    return PrimitivePathGraphicsItem::shape() + mOriginCrossGraphicsItem->shape();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void StrokeTextGraphicsItem::strokeTextLayerNameChanged(const QString& newLayerName) noexcept
{
    updateLayer(newLayerName);
}

void StrokeTextGraphicsItem::strokeTextTextChanged(const QString& newText) noexcept
{
    Q_UNUSED(newText);
    // do nothing because strokeTextPathsChanged() will be called too
}

void StrokeTextGraphicsItem::strokeTextPositionChanged(const Point& newPos) noexcept
{
    setPosition(newPos);
}

void StrokeTextGraphicsItem::strokeTextRotationChanged(const Angle& newRot) noexcept
{
    Q_UNUSED(newRot);
    updateTransform();
}

void StrokeTextGraphicsItem::strokeTextHeightChanged(const Length& newHeight) noexcept
{
    Q_UNUSED(newHeight);
    // do nothing because strokeTextPathsChanged() will be called too
}

void StrokeTextGraphicsItem::strokeTextStrokeWidthChanged(const Length& newStrokeWidth) noexcept
{
    // only line width must be updated because strokeTextPathsChanged() will be called too
    setLineWidth(newStrokeWidth);
}

void StrokeTextGraphicsItem::strokeTextLetterSpacingChanged(const StrokeTextSpacing& spacing) noexcept
{
    Q_UNUSED(spacing);
    // do nothing because strokeTextPathsChanged() will be called too
}

void StrokeTextGraphicsItem::strokeTextLineSpacingChanged(const StrokeTextSpacing& spacing) noexcept
{
    Q_UNUSED(spacing);
    // do nothing because strokeTextPathsChanged() will be called too
}

void StrokeTextGraphicsItem::strokeTextAlignChanged(const Alignment& newAlign) noexcept
{
    Q_UNUSED(newAlign);
    // do nothing because strokeTextPathsChanged() will be called too
}

void StrokeTextGraphicsItem::strokeTextMirroredChanged(bool mirrored) noexcept
{
    Q_UNUSED(mirrored);
    updateTransform();
}

void StrokeTextGraphicsItem::strokeTextAutoRotateChanged(bool newAutoRotate) noexcept
{
    Q_UNUSED(newAutoRotate);
    // do nothing because strokeTextPathsChanged() will be called too
}

void StrokeTextGraphicsItem::strokeTextPathsChanged(const QVector<Path>& paths) noexcept
{
    setPath(Path::toQPainterPathPx(paths));
}

void StrokeTextGraphicsItem::updateLayer(const QString& layerName) noexcept
{
    const GraphicsLayer* layer = mLayerProvider.getLayer(layerName);
    setLineLayer(layer);
    mOriginCrossGraphicsItem->setLayer(layer);
}

void StrokeTextGraphicsItem::updateTransform() noexcept
{
    QTransform t;
    if (mText.getMirrored()) t.scale(qreal(-1), qreal(1));
    t.rotate(-mText.getRotation().toDeg());
    setTransform(t);
}

QVariant StrokeTextGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) noexcept
{
    if (change == ItemSelectedChange && mOriginCrossGraphicsItem) {
        mOriginCrossGraphicsItem->setSelected(value.toBool());
    }
    return QGraphicsItem::itemChange(change, value);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
