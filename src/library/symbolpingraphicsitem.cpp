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
    setPos(mPin.getPosition().toPxQPointF());
    setRotation(mPin.getAngle().toDeg());
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

    Angle absAngle = mPin.getAngle();
    if (mPinInstance) absAngle += mPinInstance->getSymbolInstance().getAngle();
    absAngle.mapTo180deg();
    bool rotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    QString text;
    SchematicLayer* layer = 0;
    const GenCompSignal* genCompSignal = 0;
    const project::NetSignal* netsignal = 0;
    if (mPinInstance)
    {
        const GenCompSymbVarItem& item = mPinInstance->getSymbolInstance().getGenCompSymbVarItem();
        genCompSignal = mPinInstance->getGenCompSignal();
        netsignal = genCompSignal ? mPinInstance->getGenCompSignalInstance()->getNetSignal() : 0;
        switch (item.getDisplayTypeOfPin(mPin.getUuid()))
        {
            case GenCompSymbVarItem::DisplayType_PinName:
                text = mPin.getName(); break;
            case GenCompSymbVarItem::DisplayType_GenCompSignal:
                if (genCompSignal) text = genCompSignal->getName(); break;
            case GenCompSymbVarItem::DisplayType_NetSignal:
                if (netsignal) text = netsignal->getName(); break;
            default: break;
        }
    }
    else
        text = mPin.getName();

    // line
    layer = getSchematicLayer(SchematicLayer::SymbolOutlines);
    if (layer)
    {
        painter->setPen(QPen(layer->getColor(selected), Length(254000).toPx(), Qt::SolidLine, Qt::RoundCap)); // TODO width
        painter->drawLine(QPointF(0, 0), Point(0, mPin.getLength()).toPxQPointF());
    }

    // net signal name or circle
    layer = getSchematicLayer(SchematicLayer::SymbolPinCircles);
    if ((layer) && (!deviceIsPrinter))
    {
        painter->save();
        painter->rotate(rotate180 ? (qreal)90 : (qreal)-90);
        painter->setPen(QPen(layer->getColor(selected), 0));
        painter->setBrush(Qt::NoBrush);
        if (netsignal)
        {
            if (Workspace::instance().getSettings().getDebugTools()->getShowSymbolPinNetsignals())
            {
                QFont font;
                font.setFamily("Monospace");
                font.setPixelSize(3);
                font.setStyleHint(QFont::TypeWriter);
                font.setStyleStrategy(QFont::ForceOutline);
                painter->setFont(font);
                painter->drawText(QRectF(), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextSingleLine | Qt::TextDontClip, netsignal->getName());
            }
        }
        else
        {
            qreal radius = project::SchematicNetPoint::getCircleRadius().toPx();
            painter->drawEllipse(QPointF(0, 0), radius, radius);
        }
        painter->restore();
    }

    // text
    layer = getSchematicLayer(SchematicLayer::SymbolPinNames);
    if ((layer) && (!text.isEmpty()))
    {
        painter->save();
        painter->rotate(rotate180 ? (qreal)90 : (qreal)-90);
        painter->setPen(QPen(layer->getColor(selected), 0));
        QFont font;
        font.setFamily("Monospace");
        font.setPixelSize(4);
        font.setStyleHint(QFont::TypeWriter);
        font.setStyleStrategy(QFont::ForceOutline);
        QFontMetrics metrics(font);
        painter->setFont(font);
        qreal x = (mPin.getLength().toPx() + (metrics.width(text)/2) + 4) - metrics.width(text)/2;
        int flags = Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip;
        if (rotate180) flags |= Qt::AlignRight; else flags |= Qt::AlignLeft;
        painter->drawText(QRectF(rotate180 ? -x : x, 0, 0, 0), flags, text);
        painter->restore();
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
