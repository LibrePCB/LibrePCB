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
#include <QtWidgets>
#include <QPrinter>
#include "sgi_netlabel.h"
#include "../items/si_netlabel.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include "../../../common/schematiclayer.h"
#include "../../../workspace/workspace.h"
#include "../../../workspace/settings/workspacesettings.h"

namespace project {

QVector<QLineF> SGI_NetLabel::sOriginCrossLines;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_NetLabel::SGI_NetLabel(SI_NetLabel& netlabel) noexcept :
    SGI_Base(), mNetLabel(netlabel), mOriginCrossLayer(nullptr), mTextLayer(nullptr)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetLabels);

    mOriginCrossLayer = mNetLabel.getSchematic().getProject().getSchematicLayer(SchematicLayer::OriginCrosses);
    Q_ASSERT(mOriginCrossLayer);
    mTextLayer = mNetLabel.getSchematic().getProject().getSchematicLayer(SchematicLayer::NetLabels);
    Q_ASSERT(mTextLayer);

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

    updateCacheAndRepaint();
}

SGI_NetLabel::~SGI_NetLabel() noexcept
{
}

void SGI_NetLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const bool selected = option->state & QStyle::State_Selected;
    const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if ((lod > 2) && (!deviceIsPrinter))
    {
        // draw origin cross
        painter->setPen(QPen(mOriginCrossLayer->getColor(selected), 0));
        painter->drawLines(sOriginCrossLines);
    }

    if ((deviceIsPrinter) || (lod > 1))
    {
        // draw text
        if (mRotate180)
        {
            painter->save();
            painter->rotate(180);
        }
        painter->setPen(QPen(mTextLayer->getColor(selected), 0));
        painter->setFont(mFont);
        painter->drawText(QRectF(0, -0.5, 0, 0), mFlags, mNetLabel.getNetSignal().getName());
        if (mRotate180)
            painter->restore();
    }
    else
    {
        // draw filled rect
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(mTextLayer->getColor(selected), Qt::Dense5Pattern));
        painter->drawRect(mBoundingRect);
    }

#ifdef QT_DEBUG
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect()))
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
#endif
}

void SGI_NetLabel::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    mRotate180 = (mNetLabel.getAngle() < -Angle::deg90() || mNetLabel.getAngle() >= Angle::deg90());

    mFlags = Qt::AlignBottom | Qt::TextSingleLine | Qt::TextDontClip;
    if (mRotate180) mFlags |= Qt::AlignRight; else mFlags |= Qt::AlignLeft;

    QFontMetricsF metrics(mFont);
    QRectF rect = metrics.boundingRect(QRectF(0, -0.5, 0, 0), mFlags, mNetLabel.getNetSignal().getName());
    if (mRotate180)
        rect = QRectF(-rect.left(), -rect.top(), -rect.width(), -rect.height());
    else
        rect = QRectF(rect.left(), rect.top(), rect.width(), rect.height());
    qreal len = sOriginCrossLines[0].length();
    mBoundingRect = rect.united(QRectF(-len/2, -len/2, len, len)).normalized();

    update();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
