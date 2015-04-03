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
#include "schematicnetlabel.h"
#include "schematic.h"
#include "../circuit/netsignal.h"
#include "../circuit/circuit.h"
#include "../project.h"
#include "../../common/file_io/xmldomelement.h"
#include "../../common/schematiclayer.h"
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"

namespace project {

/*****************************************************************************************
 *  Class SchematicNetLabelGraphicsItem
 ****************************************************************************************/

QVector<QLineF> SchematicNetLabelGraphicsItem::sOriginCrossLines;

// Constructors / Destructor
SchematicNetLabelGraphicsItem::SchematicNetLabelGraphicsItem(Schematic& schematic,
                                                             SchematicNetLabel& label) noexcept :
    QGraphicsItem(), mSchematic(schematic), mLabel(label), mOriginCrossLayer(nullptr),
    mTextLayer(nullptr)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetLabels);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    mOriginCrossLayer = mSchematic.getProject().getSchematicLayer(SchematicLayer::OriginCrosses);
    Q_ASSERT(mOriginCrossLayer);
    mTextLayer = mSchematic.getProject().getSchematicLayer(SchematicLayer::NetLabels);
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

SchematicNetLabelGraphicsItem::~SchematicNetLabelGraphicsItem() noexcept
{
}

void SchematicNetLabelGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
        painter->drawText(QRectF(0, -0.5, 0, 0), mFlags, mLabel.getNetSignal().getName());
        if (mRotate180)
        {
            painter->restore();
        }
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

void SchematicNetLabelGraphicsItem::updateCacheAndRepaint() noexcept
{
    mRotate180 = (mLabel.getAngle() < -Angle::deg90() || mLabel.getAngle() >= Angle::deg90());

    mFlags = Qt::AlignBottom | Qt::TextSingleLine | Qt::TextDontClip;
    if (mRotate180) mFlags |= Qt::AlignRight; else mFlags |= Qt::AlignLeft;

    QFontMetricsF metrics(mFont);
    QRectF rect = metrics.boundingRect(QRectF(0, -0.5, 0, 0), mFlags, mLabel.getNetSignal().getName());
    if (mRotate180)
        rect = QRectF(-rect.left(), -rect.top(), -rect.width(), -rect.height());
    else
        rect = QRectF(rect.left(), rect.top(), rect.width(), rect.height());
    qreal len = sOriginCrossLines[0].length();
    mBoundingRect = rect.united(QRectF(-len/2, -len/2, len, len)).normalized();

    update();
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicNetLabel::SchematicNetLabel(Schematic& schematic, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic),
    mGraphicsItem(nullptr), mNetSignal(nullptr)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    QUuid netSignalUuid = domElement.getAttribute<QUuid>("netsignal", true);
    mNetSignal = mSchematic.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if(!mNetSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, netSignalUuid.toString(),
            QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toString()));
    }
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mAngle = domElement.getAttribute<Angle>("angle");

    init();
}

SchematicNetLabel::SchematicNetLabel(Schematic& schematic, NetSignal& netsignal, const Point& position) throw (Exception) :
    QObject(nullptr), mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic),
    mGraphicsItem(nullptr), mPosition(position), mAngle(0), mNetSignal(&netsignal)
{
    mUuid = QUuid::createUuid(); // generate random UUID
    init();
}

void SchematicNetLabel::init() throw (Exception)
{
    // create the graphics item
    mGraphicsItem = new SchematicNetLabelGraphicsItem(mSchematic, *this);
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(mAngle.toDeg());

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SchematicNetLabel::~SchematicNetLabel() noexcept
{
    delete mGraphicsItem;           mGraphicsItem = nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicNetLabel::setNetSignal(NetSignal& netsignal) noexcept
{
    if (&netsignal == mNetSignal) return;
    mNetSignal->unregisterSchematicNetLabel(*this);
    mNetSignal = &netsignal;
    mNetSignal->registerSchematicNetLabel(*this);
    mGraphicsItem->updateCacheAndRepaint();
}

void SchematicNetLabel::setPosition(const Point& position) noexcept
{
    if (position == mPosition) return;
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF());
}

void SchematicNetLabel::setAngle(const Angle& angle) noexcept
{
    if (angle == mAngle) return;
    mAngle = angle;
    mGraphicsItem->setRotation(mAngle.toDeg());
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicNetLabel::updateText() noexcept
{
    mGraphicsItem->updateCacheAndRepaint();
}

void SchematicNetLabel::addToSchematic() throw (Exception)
{
    mNetSignal->registerSchematicNetLabel(*this);
    mSchematic.addItem(mGraphicsItem);
}

void SchematicNetLabel::removeFromSchematic() throw (Exception)
{
    mNetSignal->unregisterSchematicNetLabel(*this);
    mSchematic.removeItem(mGraphicsItem);
}

XmlDomElement* SchematicNetLabel::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("netlabel"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("x", mPosition.getX());
    root->setAttribute("y", mPosition.getY());
    root->setAttribute("angle", mAngle);
    root->setAttribute("netsignal", mNetSignal->getUuid());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SchematicNetLabel::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                             return false;
    if (mNetSignal == nullptr)                      return false;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

uint SchematicNetLabel::extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                                 QList<SchematicNetLabel*>& netlabels) noexcept
{
    foreach (QGraphicsItem* item, items)
    {
        Q_ASSERT(item); if (!item) continue;
        if (item->type() == CADScene::Type_SchematicNetLabel)
        {
            SchematicNetLabelGraphicsItem* i = qgraphicsitem_cast<SchematicNetLabelGraphicsItem*>(item);
            Q_ASSERT(i); if (!i) break;
            SchematicNetLabel* l = &i->getNetLabel();
            if (!netlabels.contains(l))
                netlabels.append(l);
        }
    }
    return netlabels.count();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
