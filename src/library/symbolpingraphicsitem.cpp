/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QPrinter>
#include "symbolpingraphicsitem.h"
#include "../project/schematics/schematic.h"
#include "symbolgraphicsitem.h"
#include "symbolpin.h"
#include "../project/schematics/symbolinstance.h"
#include "../project/schematics/symbolpininstance.h"
#include "../project/project.h"
#include "../project/schematics/schematic.h"
#include "../common/schematiclayer.h"
#include "../common/units/all_length_units.h"
#include "../project/circuit/netsignal.h"
#include "../project/circuit/gencompsignalinstance.h"
#include "genericcomponent.h"
#include "../workspace/workspace.h"
#include "../project/schematics/schematicnetpoint.h"
#include "../workspace/settings/workspacesettings.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPinGraphicsItem::SymbolPinGraphicsItem(SymbolGraphicsItem& symbol, const SymbolPin& pin,
                                             project::SymbolPinInstance* instance) throw (Exception) :
    QGraphicsItem(&symbol), mSymbolGraphicsItem(symbol), mPin(pin), mPinInstance(instance)
{
    setZValue(project::Schematic::ZValue_Symbols);
    //if (mPinInstance) setFlags(QGraphicsItem::ItemIsSelectable);
    setPos(mPin.getPosition().toPxQPointF());
    setRotation(mPin.getAngle().toDeg());
    setToolTip(mPin.getName() % ": " % mPin.getDescription());

    mCircleLayer = getSchematicLayer(SchematicLayer::SymbolPinCircles);
    Q_ASSERT(mCircleLayer);
    mLineLayer = getSchematicLayer(SchematicLayer::SymbolOutlines);
    Q_ASSERT(mLineLayer);
    mTextLayer = getSchematicLayer(SchematicLayer::SymbolPinNames);
    Q_ASSERT(mTextLayer);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");
    mFont.setPixelSize(5);

    updateCacheAndRepaint();

    if (mPinInstance)
        mPinInstance->registerPinGraphicsItem(this);
}

SymbolPinGraphicsItem::~SymbolPinGraphicsItem() noexcept
{
    if (mPinInstance)
        mPinInstance->unregisterPinGraphicsItem(this);
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SymbolPinGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                                  QWidget* widget)
{
    Q_UNUSED(widget);

    const bool selected = (option->state & QStyle::State_Selected) || mSymbolGraphicsItem.isSelected();
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    const GenCompSignal* genCompSignal = (mPinInstance ? mPinInstance->getGenCompSignal() : 0);
    const project::NetSignal* netsignal = (genCompSignal ? mPinInstance->getGenCompSignalInstance()->getNetSignal() : 0);
    bool requiredPin = (mPinInstance) ? mPinInstance->getGenCompSignal()->isRequired() : false;

    // draw line
    QPen pen(mLineLayer->getColor(selected), Length(158750).toPx()*lod, Qt::SolidLine, Qt::RoundCap);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawLine(QPointF(0, 0), Point(0, mPin.getLength()).toPxQPointF());

    // draw circle
    if ((!deviceIsPrinter) && (!netsignal))
    {
        qreal radius = project::SchematicNetPoint::getCircleRadius().toPx();
        painter->setPen(QPen(mCircleLayer->getColor(requiredPin), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), radius, radius);
    }

    Angle absAngle = mPin.getAngle();
    if (mPinInstance) absAngle += mPinInstance->getSymbolInstance().getAngle();
    absAngle.mapTo180deg();

    // draw text or filled rect
    if (!mText.isEmpty())
    {
        if ((deviceIsPrinter) || (lod > 1))
        {
            // draw text
            painter->save();
            painter->rotate(mRotate180 ? 90 : -90);
            painter->setPen(QPen(mTextLayer->getColor(selected), 0));
            painter->setFont(mFont);
            qreal x = mPin.getLength().toPx() + 4;
            painter->drawText(QRectF(mRotate180 ? -x : x, 0, 0, 0), mFlags, mText);
            painter->restore();
        }
        else
        {
            // draw filled rect
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(mTextLayer->getColor(selected), Qt::Dense5Pattern));
            painter->drawRect(mTextBoundingRect);
        }
    }

#ifdef QT_DEBUG
    if ((!deviceIsPrinter) && (netsignal) && (Workspace::instance().getSettings().getDebugTools()->getShowSymbolPinNetsignals()))
    {
        // draw net signal name
        QFont font;
        font.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
        font.setStyleHint(QFont::TypeWriter);
        font.setFamily("Monospace");
        font.setPixelSize(3);
        painter->setFont(font);
        painter->setPen(QPen(mCircleLayer->getColor(requiredPin), 0));
        painter->save();
        painter->rotate(mRotate180 ? 90 : -90);
        painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextSingleLine | Qt::TextDontClip, netsignal->getName());
        painter->restore();
    }
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect()))
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolPinGraphicsItem::updateCacheAndRepaint() noexcept
{
    mText = (mPinInstance ? mPinInstance->getDisplayText() : mPin.getName());

    Angle absAngle = mPin.getAngle();
    if (mPinInstance) absAngle += mPinInstance->getSymbolInstance().getAngle();
    absAngle.mapTo180deg();
    mRotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    mFlags = Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip;
    if (mRotate180) mFlags |= Qt::AlignRight; else mFlags |= Qt::AlignLeft;

    mShape = QPainterPath();
    mShape.setFillRule(Qt::WindingFill);
    mBoundingRect = QRectF();

    // circle
    qreal radius = project::SchematicNetPoint::getCircleRadius().toPx();
    mShape.addEllipse(-radius, -radius, 2*radius, 2*radius);
    mBoundingRect = mBoundingRect.united(mShape.boundingRect());

    // line
    QRectF lineRect = QRectF(QPointF(0, 0), Point(0, mPin.getLength()).toPxQPointF()).normalized();
    lineRect.adjust(-Length(79375).toPx(), -Length(79375).toPx(), Length(79375).toPx(), Length(79375).toPx());
    mBoundingRect = mBoundingRect.united(lineRect).normalized();

    // text
    qreal x = mPin.getLength().toPx() + 4;
    QFontMetricsF metrics(mFont);
    QRectF textRect = metrics.boundingRect(QRectF(mRotate180 ? -x : x, 0, 0, 0), mFlags, mText).normalized();
    if (mRotate180)
        textRect = QRectF(textRect.top(), textRect.left(), textRect.height(), textRect.width());
    else
        textRect = QRectF(-textRect.top(), -textRect.left(), -textRect.height(), -textRect.width());
    mTextBoundingRect = textRect.normalized();
    mBoundingRect = mBoundingRect.united(mTextBoundingRect).normalized();

    update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SchematicLayer* SymbolPinGraphicsItem::getSchematicLayer(unsigned int id) const noexcept
{
    if (mPinInstance)
        return mPinInstance->getSymbolInstance().getSchematic().getProject().getSchematicLayer(id);
    else
        return Workspace::instance().getSchematicLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
