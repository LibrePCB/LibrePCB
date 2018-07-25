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
#include <QPrinter>
#include "symbolpinpreviewgraphicsitem.h"
#include "symbolpin.h"
#include "../cmp/component.h"
#include <librepcb/common/application.h>
#include <librepcb/common/graphics/graphicslayer.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPinPreviewGraphicsItem::SymbolPinPreviewGraphicsItem(const IF_GraphicsLayerProvider& layerProvider, const SymbolPin& pin,
        const ComponentSignal* compSignal, const CmpSigPinDisplayType& displayType) noexcept :
    QGraphicsItem(), mPin(pin), mComponentSignal(compSignal), mDisplayType(displayType),
    mDrawBoundingRect(false)
{
    setToolTip(mPin.getName());

    if (mComponentSignal && mComponentSignal->isRequired()) {
        mCircleLayer = layerProvider.getLayer(GraphicsLayer::sSymbolPinCirclesReq);
    } else {
        mCircleLayer = layerProvider.getLayer(GraphicsLayer::sSymbolPinCirclesOpt);
    }
    Q_ASSERT(mCircleLayer);
    mLineLayer = layerProvider.getLayer(GraphicsLayer::sSymbolOutlines);
    Q_ASSERT(mLineLayer);
    mTextLayer = layerProvider.getLayer(GraphicsLayer::sSymbolPinNames);
    Q_ASSERT(mTextLayer);

    mStaticText.setTextFormat(Qt::PlainText);
    mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

    mFont = qApp->getDefaultSansSerifFont();
    mFont.setPixelSize(5);

    mRadiusPx = Length(600000).toPx();

    updateCacheAndRepaint();
}

SymbolPinPreviewGraphicsItem::~SymbolPinPreviewGraphicsItem() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPinPreviewGraphicsItem::updateCacheAndRepaint() noexcept
{
    mShape = QPainterPath();
    mShape.setFillRule(Qt::WindingFill);
    mBoundingRect = QRectF();

    // rotation
    Angle absAngle = mPin.getRotation() + Angle::fromDeg(parentItem() ? -parentItem()->rotation() : 0);
    absAngle.mapTo180deg();
    mRotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    // circle
    mShape.addEllipse(-mRadiusPx, -mRadiusPx, 2*mRadiusPx, 2*mRadiusPx);
    mBoundingRect = mBoundingRect.united(mShape.boundingRect());

    // line
    QRectF lineRect = QRectF(QPointF(0, 0), Point(*mPin.getLength(), 0).toPxQPointF()).normalized();
    lineRect.adjust(-Length(79375).toPx(), -Length(79375).toPx(), Length(79375).toPx(), Length(79375).toPx());
    mBoundingRect = mBoundingRect.united(lineRect).normalized();

    // text
    if (mDisplayType == CmpSigPinDisplayType::none()) {
        mStaticText.setText("");
    } else if (mDisplayType == CmpSigPinDisplayType::pinName()) {
        mStaticText.setText(mPin.getName());
    } else  if (mDisplayType == CmpSigPinDisplayType::componentSignal()) {
        mStaticText.setText(mComponentSignal ? mComponentSignal->getName() : "");
    } else if (mDisplayType == CmpSigPinDisplayType::netSignal()) {
        mStaticText.setText(mComponentSignal ? mComponentSignal->getForcedNetName() : "");
    } else {
        Q_ASSERT(false);
        mStaticText.setText("???");
    }
    qreal x = mPin.getLength()->toPx() + 4;
    mStaticText.prepare(QTransform(), mFont);
    mTextOrigin.setX(mRotate180 ? -x-mStaticText.size().width() : x);
    mTextOrigin.setY(-mStaticText.size().height()/2);
    mStaticText.prepare(QTransform().rotate(mRotate180 ? 180 : 0)
                                    .translate(mTextOrigin.x(), mTextOrigin.y()), mFont);
    if (mRotate180)
        mTextBoundingRect = QRectF(-mTextOrigin.x(), -mTextOrigin.y(), -mStaticText.size().width(), -mStaticText.size().height()).normalized();
    else
        mTextBoundingRect = QRectF(mTextOrigin.x(), -mTextOrigin.y()-mStaticText.size().height(), mStaticText.size().width(), mStaticText.size().height()).normalized();
    mBoundingRect = mBoundingRect.united(mTextBoundingRect).normalized();

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SymbolPinPreviewGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(widget);
    const bool selected = option->state.testFlag(QStyle::State_Selected);

    // draw line
    QPen pen(mLineLayer->getColor(selected), Length(158750).toPx(), Qt::SolidLine, Qt::RoundCap);
    painter->setPen(pen);
    painter->drawLine(QPointF(0, 0), Point(*mPin.getLength(), 0).toPxQPointF());

    // draw circle
    painter->setPen(QPen(mCircleLayer->getColor(selected), 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(0, 0), mRadiusPx, mRadiusPx);

    // draw text
    painter->save();
    if (mRotate180) painter->rotate(180);
    painter->setPen(QPen(mTextLayer->getColor(selected), 0));
    painter->setFont(mFont);
    painter->drawStaticText(mTextOrigin, mStaticText);
    painter->restore();

#ifdef QT_DEBUG
    if (mDrawBoundingRect)
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
