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
#include "primitivetextgraphicsitem.h"
#include "../application.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PrimitiveTextGraphicsItem::PrimitiveTextGraphicsItem(QGraphicsItem* parent) noexcept :
    QGraphicsItem(parent), mLayer(nullptr), mAlignment(HAlign::left(), VAlign::bottom()),
    mTextFlags(0)
{
    mFont = qApp->getDefaultSansSerifFont();
    mFont.setPixelSize(1);

    updateBoundingRectAndShape();
    setVisible(false);
}

PrimitiveTextGraphicsItem::~PrimitiveTextGraphicsItem() noexcept
{
    // unregister from graphics layer
    setLayer(nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void PrimitiveTextGraphicsItem::setPosition(const Point& pos) noexcept
{
    QGraphicsItem::setPos(pos.toPxQPointF());
}

void PrimitiveTextGraphicsItem::setRotation(const Angle& rot) noexcept
{
    QGraphicsItem::setRotation(-rot.toDeg());
}

void PrimitiveTextGraphicsItem::setText(const QString& text) noexcept
{
    mText = text;
    updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setHeight(const Length& height) noexcept
{
    mFont.setPixelSize(height.toPx());
    updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setAlignment(const Alignment& align) noexcept
{
    mAlignment = align;
    updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setFont(Font font) noexcept
{
    int size = mFont.pixelSize(); // memorize size
    switch (font) {
        case Font::SansSerif: mFont = qApp->getDefaultSansSerifFont(); break;
        case Font::Monospace: mFont = qApp->getDefaultMonospaceFont(); break;
        default: {
            Q_ASSERT(false);
            qCritical() << "Unknown font:" << static_cast<int>(font);
            break;
        }
    }
    mFont.setPixelSize(size);
    updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setLayer(const GraphicsLayer* layer) noexcept
{
    if (mLayer) {
        mLayer->unregisterObserver(*this);
    }
    mLayer = layer;
    if (mLayer) {
        mLayer->registerObserver(*this);
        mPen.setColor(mLayer->getColor(false));
        mPenHighlighted.setColor(mLayer->getColor(true));
        setVisible(mLayer->isVisible());
        update();
    } else {
        setVisible(false);
    }
}

/*****************************************************************************************
 *  Inherited from IF_LayerObserver
 ****************************************************************************************/

void PrimitiveTextGraphicsItem::layerColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept
{
    Q_UNUSED(layer);
    Q_ASSERT(&layer == mLayer);
    mPen.setColor(newColor);
    update();
}

void PrimitiveTextGraphicsItem::layerHighlightColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept
{
    Q_UNUSED(layer);
    Q_ASSERT(&layer == mLayer);
    mPenHighlighted.setColor(newColor);
    update();
}

void PrimitiveTextGraphicsItem::layerVisibleChanged(const GraphicsLayer& layer, bool newVisible) noexcept
{
    Q_ASSERT(&layer == mLayer);
    Q_UNUSED(newVisible);
    setVisible(layer.isVisible());
}

void PrimitiveTextGraphicsItem::layerEnabledChanged(const GraphicsLayer& layer, bool newEnabled) noexcept
{
    Q_ASSERT(&layer == mLayer);
    layerVisibleChanged(layer, newEnabled);
}

void PrimitiveTextGraphicsItem::layerDestroyed(const GraphicsLayer& layer) noexcept
{
    Q_ASSERT(&layer == mLayer);
    setLayer(nullptr);
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void PrimitiveTextGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(widget);
    painter->setFont(mFont);
    if (option->state.testFlag(QStyle::State_Selected)) {
        painter->setPen(mPenHighlighted);
    } else {
        painter->setPen(mPen);
    }

    if (mapToScene(0, 1).y() < mapToScene(0, 0).y()) {
        // The text needs to be rotated 180°!
        // TODO: Is there a better solution to determine the overall rotation of the item?
        //painter->save();
        painter->rotate(180);
        painter->translate(-mBoundingRect.topLeft() - mBoundingRect.bottomRight());
        painter->drawText(QRectF(), mTextFlags, mText);
        //painter->restore();
    } else {
        painter->drawText(QRectF(), mTextFlags, mText);
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PrimitiveTextGraphicsItem::updateBoundingRectAndShape() noexcept
{
    prepareGeometryChange();
    mTextFlags = Qt::TextDontClip | mAlignment.toQtAlign();
    QFontMetricsF fm(mFont);
    mBoundingRect = fm.boundingRect(QRectF(), mTextFlags, mText);
    mShape = QPainterPath();
    mShape.addRect(mBoundingRect);
    update();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
