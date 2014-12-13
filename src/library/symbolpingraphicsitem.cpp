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
#include "../project/circuit/netsignal.h"
#include "../project/circuit/gencompsignalinstance.h"
#include "genericcomponent.h"
#include "../workspace/workspace.h"
#include "../project/schematics/schematicnetpoint.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPinGraphicsItem::SymbolPinGraphicsItem(SymbolGraphicsItem& symbol, const SymbolPin& pin,
                                             project::SymbolPinInstance* instance) throw (Exception) :
    QGraphicsItem(&symbol), mSymbolGraphicsItem(symbol), mPin(pin), mPinInstance(instance)
{
    setZValue(project::Schematic::ZValue_Symbols);
    setPos(pin.getPosition().toPxQPointF());
    setToolTip(mPin.getName() % ": " % mPin.getDescription());

    if (!updateBoundingRectAndShape())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QApplication::translate("SymbolPinGraphicsItem", "Invalid Symbol Pin"));
    }

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

    bool selected = (option->state & QStyle::State_Selected) || mSymbolGraphicsItem.isSelected();
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);

    SchematicLayer* layer = 0;
    const GenCompSignal* genCompSignal = 0;
    const project::NetSignal* netsignal = 0;
    if (mPinInstance)
    {
        genCompSignal = mPinInstance->getGenCompSignal();
        netsignal = genCompSignal ? mPinInstance->getGenCompSignalInstance()->getNetSignal() : 0;
    }
    QString text = (genCompSignal) ? genCompSignal->getName() : mPin.getName();

    // line
    layer = getSchematicLayer(SchematicLayer::SymbolOutlines);
    if (layer)
    {
        painter->setPen(QPen(layer->getColor(selected), Length(254000).toPx(), Qt::SolidLine, Qt::RoundCap)); // TODO width
        Point p2(mPin.getLength() * qSin(mPin.getAngle().toRad()),
                 mPin.getLength() * qCos(mPin.getAngle().toRad()));
        painter->drawLine(QPointF(0, 0), p2.toPxQPointF());
    }

    // net signal name or circle
    layer = getSchematicLayer(SchematicLayer::SymbolPinCircles);
    if ((layer) && (!deviceIsPrinter))
    {
        painter->setPen(QPen(layer->getColor(selected), 0));
        painter->setBrush(Qt::NoBrush);
        if (netsignal)
        {
            QFont font;
            font.setFamily("Monospace");
            font.setPixelSize(3);
            font.setStyleHint(QFont::TypeWriter);
            font.setStyleStrategy(QFont::ForceOutline);
            painter->setFont(font);
            painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextSingleLine | Qt::TextDontClip, netsignal->getName());
        }
        else
        {
            qreal radius = project::SchematicNetPoint::getCircleRadius().toPx();
            painter->drawEllipse(QPointF(0, 0), radius, radius);
        }
    }

    // text
    layer = getSchematicLayer(SchematicLayer::SymbolPinNames);
    if (layer)
    {
        painter->setPen(QPen(layer->getColor(selected), 0));
        QFont font;
        font.setFamily("Monospace");
        font.setPixelSize(4);
        font.setStyleHint(QFont::TypeWriter);
        font.setStyleStrategy(QFont::ForceOutline);
        QFontMetrics metrics(font);
        painter->setFont(font);
        QRectF rect(QPointF((mPin.getLength().toPx() + (metrics.width(text)/2) + 4)
                            * qSin(mPin.getAngle().toRad()) - metrics.width(text)/2,
                            0), QSizeF(0, 0));
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip, text);
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolPinGraphicsItem::updateBoundingRectAndShape() noexcept
{
    QRectF boundingRect;
    QPainterPath shape;
    shape.setFillRule(Qt::WindingFill);
    qreal radius = project::SchematicNetPoint::getCircleRadius().toPx();

    // circle
    shape.addEllipse(-radius, -radius, 2*radius, 2*radius);
    boundingRect = shape.boundingRect();

    // line
    Point p2(mPin.getLength() * qSin(mPin.getAngle().toRad()),
             mPin.getLength() * qCos(mPin.getAngle().toRad()));
    boundingRect = boundingRect.united(QRectF(QPointF(0, 0), p2.toPxQPointF()).normalized());

    mBoundingRect = boundingRect.adjusted(-3, -3, 3, 3);
    mShape = shape;
    return true;
}

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
