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
#include "footprintpadgraphicsitem.h"
#include "footprintpad.h"
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/primitivepathgraphicsitem.h>
//#include <librepcb/common/graphics/primitivetextgraphicsitem.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPadGraphicsItem::FootprintPadGraphicsItem(FootprintPad& pad,
        const IF_GraphicsLayerProvider& lp, QGraphicsItem* parent) noexcept :
    QGraphicsItem(parent), mPad(pad), mLayerProvider(lp),
    mPathGraphicsItem(new PrimitivePathGraphicsItem(this))/*,
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this))*/
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(10);

    // path properties
    mPathGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    // pin properties
    setPosition(mPad.getPosition());
    setRotation(mPad.getRotation());
    setShape(mPad.toQPainterPathPx());
    setLayerName(mPad.getLayerName());

    // register to the pad to get attribute updates
    mPad.registerGraphicsItem(*this);
}

FootprintPadGraphicsItem::~FootprintPadGraphicsItem() noexcept
{
    mPad.unregisterGraphicsItem(*this);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FootprintPadGraphicsItem::setPosition(const Point& pos) noexcept
{
    QGraphicsItem::setPos(pos.toPxQPointF());
}

void FootprintPadGraphicsItem::setRotation(const Angle& rot) noexcept
{
    QGraphicsItem::setRotation(-rot.toDeg());
}

void FootprintPadGraphicsItem::setShape(const QPainterPath& shape) noexcept
{
    mPathGraphicsItem->setPath(shape);
    //mTextGraphicsItem->setHeight(Length::fromPx(shape.boundingRect().height()));
}

void FootprintPadGraphicsItem::setLayerName(const QString& name) noexcept
{
    mPathGraphicsItem->setFillLayer(mLayerProvider.getLayer(name));
   // mTextGraphicsItem->setLayer(mLayerProvider.getLayer(id));
}

/*void FootprintPadGraphicsItem::setName(const QString& name) noexcept
{
    mTextGraphicsItem->setText(name);
}*/

void FootprintPadGraphicsItem::setSelected(bool selected) noexcept
{
    mPathGraphicsItem->setSelected(selected);
    //mTextGraphicsItem->setSelected(selected);
    QGraphicsItem::setSelected(selected);
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

QPainterPath FootprintPadGraphicsItem::shape() const noexcept
{
    return mPathGraphicsItem->shape();
}

void FootprintPadGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) noexcept
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
