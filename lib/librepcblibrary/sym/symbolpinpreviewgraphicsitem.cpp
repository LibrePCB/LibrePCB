/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "../gencmp/genericcomponent.h"
#include <librepcbcommon/schematiclayer.h>
#include <librepcbcommon/if_schematiclayerprovider.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPinPreviewGraphicsItem::SymbolPinPreviewGraphicsItem(const IF_SchematicLayerProvider& layerProvider,
                                                           const QStringList& localeOrder,
                                                           const SymbolPin& pin,
                                                           const GenCompSignal* genCompSignal,
                                                           GenCompSymbVarItem::PinDisplayType_t displayType) noexcept :
    GraphicsItem(), mPin(pin), mGenCompSignal(genCompSignal), mDisplayType(displayType),
    mDrawBoundingRect(false), mLocaleOrder(localeOrder)
{
    setToolTip(mPin.getName(mLocaleOrder) % ": " % mPin.getDescription(mLocaleOrder));

    mCircleLayer = layerProvider.getSchematicLayer(SchematicLayer::SymbolPinCircles);
    Q_ASSERT(mCircleLayer);
    mLineLayer = layerProvider.getSchematicLayer(SchematicLayer::SymbolOutlines);
    Q_ASSERT(mLineLayer);
    mTextLayer = layerProvider.getSchematicLayer(SchematicLayer::SymbolPinNames);
    Q_ASSERT(mTextLayer);

    mStaticText.setTextFormat(Qt::PlainText);
    mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");
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
    Angle absAngle = mPin.getAngle() + Angle::fromDeg(parentItem() ? parentItem()->rotation() : 0);
    mRotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    // circle
    mShape.addEllipse(-mRadiusPx, -mRadiusPx, 2*mRadiusPx, 2*mRadiusPx);
    mBoundingRect = mBoundingRect.united(mShape.boundingRect());

    // line
    QRectF lineRect = QRectF(QPointF(0, 0), Point(0, mPin.getLength()).toPxQPointF()).normalized();
    lineRect.adjust(-Length(79375).toPx(), -Length(79375).toPx(), Length(79375).toPx(), Length(79375).toPx());
    mBoundingRect = mBoundingRect.united(lineRect).normalized();

    // text
    switch (mDisplayType)
    {
        case GenCompSymbVarItem::PinDisplayType_t::None:
            mStaticText.setText(""); break;
        case GenCompSymbVarItem::PinDisplayType_t::PinName:
            mStaticText.setText(mPin.getName(mLocaleOrder)); break;
        case GenCompSymbVarItem::PinDisplayType_t::GenCompSignal:
            mStaticText.setText(mGenCompSignal ? mGenCompSignal->getName(mLocaleOrder) : ""); break;
        case GenCompSymbVarItem::PinDisplayType_t::NetSignal:
            mStaticText.setText(mGenCompSignal ? mGenCompSignal->getForcedNetName() : ""); break;
        default: Q_ASSERT(false);
    }
    qreal x = mPin.getLength().toPx() + 4;
    mStaticText.prepare(QTransform(), mFont);
    mTextOrigin.setX(mRotate180 ? -x-mStaticText.size().width() : x);
    mTextOrigin.setY(-mStaticText.size().height()/2);
    mStaticText.prepare(QTransform().rotate(mRotate180 ? 90 : -90)
                                    .translate(mTextOrigin.x(), mTextOrigin.y()), mFont);
    if (mRotate180)
        mTextBoundingRect = QRectF(mTextOrigin.y(), mTextOrigin.x(), mStaticText.size().height(), mStaticText.size().width()).normalized();
    else
        mTextBoundingRect = QRectF(mTextOrigin.y(), -mTextOrigin.x()-mStaticText.size().width(), mStaticText.size().height(), mStaticText.size().width()).normalized();
    mBoundingRect = mBoundingRect.united(mTextBoundingRect).normalized();

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SymbolPinPreviewGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    const bool selected = option->state.testFlag(QStyle::State_Selected);

    bool requiredPin = mGenCompSignal ? mGenCompSignal->isRequired() : false;

    // draw line
    QPen pen(mLineLayer->getColor(selected), Length(158750).toPx(), Qt::SolidLine, Qt::RoundCap);
    painter->setPen(pen);
    painter->drawLine(QPointF(0, 0), Point(0, mPin.getLength()).toPxQPointF());

    // draw circle
    painter->setPen(QPen(mCircleLayer->getColor(requiredPin), 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(0, 0), mRadiusPx, mRadiusPx);

    // draw text
    painter->save();
    painter->rotate(mRotate180 ? 90 : -90);
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
