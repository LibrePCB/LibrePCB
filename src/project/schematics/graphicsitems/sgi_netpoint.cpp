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
#include "sgi_netpoint.h"
#include "../items/si_netpoint.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../../common/schematiclayer.h"
#include "../../../workspace/workspace.h"
#include "../../../workspace/settings/workspacesettings.h"

namespace project {

QRectF SGI_NetPoint::sBoundingRect;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SGI_NetPoint::SGI_NetPoint(SI_NetPoint& netpoint) noexcept :
    SGI_Base(), mNetPoint(netpoint), mLayer(nullptr)
{
    setZValue(Schematic::ZValue_VisibleNetPoints);

    mLayer = mNetPoint.getSchematic().getProject().getSchematicLayer(SchematicLayer::Nets);
    Q_ASSERT(mLayer);

    if (sBoundingRect.isNull())
    {
        qreal radius = Length(600000).toPx();
        sBoundingRect = QRectF(-radius, -radius, 2*radius, 2*radius);
    }

    updateCacheAndRepaint();
}

SGI_NetPoint::~SGI_NetPoint() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SGI_NetPoint::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();
    mPointVisible = mNetPoint.isVisible();
    setZValue(mPointVisible ? Schematic::ZValue_VisibleNetPoints : Schematic::ZValue_HiddenNetPoints);
    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void SGI_NetPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (mPointVisible)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(mLayer->getColor(mNetPoint.isSelected()), Qt::SolidPattern));
        painter->drawEllipse(sBoundingRect);
    }

#ifdef QT_DEBUG
    bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    if ((!mPointVisible) && (!deviceIsPrinter) && Workspace::instance().getSettings().getDebugTools()->getShowAllSchematicNetpoints())
    {
        // draw circle
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(sBoundingRect);
    }
    if ((!deviceIsPrinter) && (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect()))
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(sBoundingRect);
    }
#endif
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
