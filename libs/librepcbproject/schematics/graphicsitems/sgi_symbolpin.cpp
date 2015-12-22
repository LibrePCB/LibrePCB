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
#include "sgi_symbolpin.h"
#include "../items/si_symbolpin.h"
#include "../items/si_symbol.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/componentsignalinstance.h"
#include <librepcbcommon/schematiclayer.h>
#include <librepcblibrary/sym/symbolpin.h>
#include <librepcblibrary/cmp/component.h>
#include "../../settings/projectsettings.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_SymbolPin::SGI_SymbolPin(SI_SymbolPin& pin) noexcept :
    SGI_Base(), mPin(pin), mLibPin(pin.getLibPin())
{
    setZValue(Schematic::ZValue_Symbols);
    setToolTip(mLibPin.getName());

    mStaticText.setTextFormat(Qt::PlainText);
    mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");
    mFont.setPixelSize(5);

    mRadiusPx = Length(600000).toPx();

    updateCacheAndRepaint();
}

SGI_SymbolPin::~SGI_SymbolPin() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SGI_SymbolPin::updateCacheAndRepaint() noexcept
{
    mShape = QPainterPath();
    mShape.setFillRule(Qt::WindingFill);
    mBoundingRect = QRectF();

    // rotation
    Angle absAngle = mLibPin.getRotation() + mPin.getSymbol().getRotation();
    absAngle.mapTo180deg();
    mRotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    // circle
    mShape.addEllipse(-mRadiusPx, -mRadiusPx, 2*mRadiusPx, 2*mRadiusPx);
    mBoundingRect = mBoundingRect.united(mShape.boundingRect());

    // line
    QRectF lineRect = QRectF(QPointF(0, 0), Point(mLibPin.getLength(), 0).toPxQPointF()).normalized();
    lineRect.adjust(-Length(79375).toPx(), -Length(79375).toPx(), Length(79375).toPx(), Length(79375).toPx());
    mBoundingRect = mBoundingRect.united(lineRect).normalized();

    // text
    qreal x = mLibPin.getLength().toPx() + 4;
    mStaticText.setText(mPin.getDisplayText());
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

void SGI_SymbolPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    const library::ComponentSignal* cmpSignal = mPin.getComponentSignal();
    const NetSignal* netsignal = (cmpSignal ? mPin.getComponentSignalInstance()->getNetSignal() : nullptr);
    bool requiredPin = mPin.getComponentSignal()->isRequired();

    // draw line
    SchematicLayer* layer = getSchematicLayer(SchematicLayer::SymbolOutlines); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        painter->setPen(QPen(layer->getColor(mPin.isSelected()), Length(158750).toPx(), Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(QPointF(0, 0), Point(mLibPin.getLength(), 0).toPxQPointF());
    }

    // draw circle
    layer = getSchematicLayer(SchematicLayer::SymbolPinCircles); Q_ASSERT(layer);
    if ((layer->isVisible()) && (!deviceIsPrinter) && (!netsignal))
    {
        painter->setPen(QPen(layer->getColor(requiredPin), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), mRadiusPx, mRadiusPx);
    }

    // draw text or filled rect
    layer = getSchematicLayer(SchematicLayer::SymbolPinNames); Q_ASSERT(layer);
    if ((layer->isVisible()) && (!mStaticText.text().isEmpty()))
    {
        if ((deviceIsPrinter) || (lod > 1))
        {
            // draw text
            painter->save();
            if (mRotate180) painter->rotate(180);
            painter->setPen(QPen(layer->getColor(mPin.isSelected()), 0));
            painter->setFont(mFont);
            painter->drawStaticText(mTextOrigin, mStaticText);
            painter->restore();
        }
        else
        {
            // draw filled rect
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(layer->getColor(mPin.isSelected()), Qt::Dense5Pattern));
            painter->drawRect(mTextBoundingRect);
        }
    }

#ifdef QT_DEBUG
    layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_SymbolPinNetSignalNames); Q_ASSERT(layer);
    if ((layer->isVisible()) && (netsignal))
    {
        // draw net signal name
        QFont font;
        font.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
        font.setStyleHint(QFont::TypeWriter);
        font.setFamily("Monospace");
        font.setPixelSize(3);
        painter->setFont(font);
        painter->setPen(QPen(layer->getColor(mPin.isSelected()), 0));
        painter->save();
        if (mRotate180) painter->rotate(180);
        painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextSingleLine | Qt::TextDontClip, netsignal->getName());
        painter->restore();
    }
    layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsBoundingRect); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(mPin.isSelected()), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
    layer = getSchematicLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsTextsBoundingRect); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw text bounding rect
        painter->setPen(QPen(layer->getColor(mPin.isSelected()), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mTextBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SchematicLayer* SGI_SymbolPin::getSchematicLayer(int id) const noexcept
{
    return mPin.getSymbol().getSchematic().getProject().getSchematicLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
