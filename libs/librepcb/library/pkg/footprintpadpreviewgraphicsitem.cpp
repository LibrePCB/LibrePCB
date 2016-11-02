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
#include "footprintpadpreviewgraphicsitem.h"
#include "packagepad.h"
#include "footprintpad.h"
#include <librepcb/common/graphics/graphicslayer.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPadPreviewGraphicsItem::FootprintPadPreviewGraphicsItem(const IF_GraphicsLayerProvider& layerProvider, const FootprintPad& fptPad,
        const PackagePad* pkgPad) noexcept :
    QGraphicsItem(), mFootprintPad(fptPad), mPackagePad(pkgPad), mDrawBoundingRect(false)
{
    if (mPackagePad)
        setToolTip(mPackagePad->getName());

    mLayer = layerProvider.getLayer(mFootprintPad.getLayerName());
    Q_ASSERT(mLayer);

    updateCacheAndRepaint();
}

FootprintPadPreviewGraphicsItem::~FootprintPadPreviewGraphicsItem() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FootprintPadPreviewGraphicsItem::updateCacheAndRepaint() noexcept
{
    mShape = mFootprintPad.toQPainterPathPx();
    mBoundingRect = mShape.boundingRect();

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void FootprintPadPreviewGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    const bool selected = option->state.testFlag(QStyle::State_Selected);

    // draw shape
    QBrush brush(mLayer->getColor(selected), Qt::SolidPattern);
    painter->setPen(Qt::NoPen);
    painter->setBrush(brush);
    painter->drawPath(mShape);

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
