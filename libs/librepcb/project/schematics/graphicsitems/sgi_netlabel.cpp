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
#include "sgi_netlabel.h"
#include "../items/si_netlabel.h"
#include "../items/si_netsegment.h"
#include "../schematic.h"
#include "../schematiclayerprovider.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include <librepcb/common/graphics/linegraphicsitem.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

QVector<QLineF> SGI_NetLabel::sOriginCrossLines;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_NetLabel::SGI_NetLabel(SI_NetLabel& netlabel) noexcept :
    SGI_Base(), mNetLabel(netlabel)
{
    setZValue(Schematic::ZValue_NetLabels);

    mStaticText.setTextFormat(Qt::PlainText);
    mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::TypeWriter);
    mFont.setFamily("Monospace");
    mFont.setPixelSize(4);

    if (sOriginCrossLines.isEmpty())
    {
        qreal crossSizePx = Length(400000).toPx();
        sOriginCrossLines.append(QLineF(-crossSizePx, 0, crossSizePx, 0));
        sOriginCrossLines.append(QLineF(0, -crossSizePx, 0, crossSizePx));
    }

    // create anchor graphics item
    mAnchorGraphicsItem.reset(new LineGraphicsItem(this));
    mAnchorGraphicsItem->setLayer(getLayer(GraphicsLayer::sSchematicNetLabelAnchors));

    updateCacheAndRepaint();
}

SGI_NetLabel::~SGI_NetLabel() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SGI_NetLabel::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    mRotate180 = (mNetLabel.getRotation().mappedTo180deg() <= -Angle::deg90()
                  || mNetLabel.getRotation().mappedTo180deg() > Angle::deg90());

    mStaticText.setText(mNetLabel.getNetSignalOfNetSegment().getName());
    mStaticText.prepare(QTransform(), mFont);
    mTextOrigin.setX(mRotate180 ? -mStaticText.size().width() : 0);
    mTextOrigin.setY(mRotate180 ? 0 : -0.5-mStaticText.size().height());
    mStaticText.prepare(QTransform().rotate(mRotate180 ? 180 : 0)
                              .translate(mTextOrigin.x(), mTextOrigin.y()), mFont);

    QRectF rect = QRectF(0, 0, mStaticText.size().width(), -mStaticText.size().height()).normalized();
    qreal len = sOriginCrossLines[0].length();
    mBoundingRect = rect.united(QRectF(-len/2, -len/2, len, len)).normalized();

    update();
}

void SGI_NetLabel::setAnchor(const Point& pos) noexcept
{
    mAnchorGraphicsItem->setLine(Point(), Point::fromPx(mapFromScene(pos.toPxQPointF())));
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SGI_NetLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    bool highlight = mNetLabel.isSelected() || mNetLabel.getNetSignalOfNetSegment().isHighlighted();

    GraphicsLayer* layer = getLayer(GraphicsLayer::sSchematicReferences); Q_ASSERT(layer);
    if ((layer->isVisible()) && (lod > 2) && (!deviceIsPrinter))
    {
        // draw origin cross
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->drawLines(sOriginCrossLines);
    }

    layer = getLayer(GraphicsLayer::sSchematicNetLabels); Q_ASSERT(layer);
    if ((layer->isVisible()) && ((deviceIsPrinter) || (lod > 1)))
    {
        // draw text
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->setFont(mFont);
        if (mRotate180)
        {
            painter->save();
            painter->rotate(180);
            painter->drawStaticText(mTextOrigin, mStaticText);
            painter->restore();
        }
        else
            painter->drawStaticText(mTextOrigin, mStaticText);
    }
    else
    {
        // draw filled rect
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(layer->getColor(highlight), Qt::Dense5Pattern));
        painter->drawRect(mBoundingRect);
    }

#ifdef QT_DEBUG
    layer = getLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw bounding rect
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
    layer = getLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects); Q_ASSERT(layer);
    if (layer->isVisible())
    {
        // draw text bounding rect
        painter->setPen(QPen(layer->getColor(highlight), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(mTextOrigin, mStaticText.size()));
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

GraphicsLayer* SGI_NetLabel::getLayer(const QString& name) const noexcept
{
    return mNetLabel.getProject().getLayers().getLayer(name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
