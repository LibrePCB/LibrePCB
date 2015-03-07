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

namespace project {

/*****************************************************************************************
 *  Class SchematicNetLabelGraphicsItem
 ****************************************************************************************/

// Constructors / Destructor
SchematicNetLabelGraphicsItem::SchematicNetLabelGraphicsItem(Schematic& schematic,
                                                             SchematicNetLabel& label) throw (Exception) :
    QGraphicsSimpleTextItem(), mSchematic(schematic), mLabel(label)
{
    SchematicLayer* layer = mSchematic.getProject().getSchematicLayer(SchematicLayer::NetLabels);
    Q_ASSERT(layer);

    QFont font;
    font.setFamily("Monospace");
    font.setPixelSize(50);
    font.setStyleHint(QFont::TypeWriter);
    font.setStyleStrategy(QFont::ForceOutline);
    setFont(font);
    setBrush(layer->getColor(false));
    setScale((qreal)0.1);
    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(Schematic::ZValue_NetPoints);
}

SchematicNetLabelGraphicsItem::~SchematicNetLabelGraphicsItem() noexcept
{
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
    mGraphicsItem->setText(mNetSignal->getName());
    setPosition(mPosition);
    setAngle(mAngle);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SchematicNetLabel::~SchematicNetLabel() noexcept
{
    delete mGraphicsItem;           mGraphicsItem = nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicNetLabel::setNetSignal(NetSignal& netsignal) throw (Exception)
{
    mNetSignal->unregisterSchematicNetLabel(*this);
    mNetSignal = &netsignal;
    mGraphicsItem->setText(mNetSignal->getName());
    mNetSignal->registerSchematicNetLabel(*this);
}

void SchematicNetLabel::setPosition(const Point& position) noexcept
{
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF() - QPointF(0, mGraphicsItem->boundingRect().height()/10));
}

void SchematicNetLabel::setAngle(const Angle& angle) noexcept
{
    mAngle = angle;
    mGraphicsItem->setRotation(mAngle.toDeg());
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicNetLabel::updateText() noexcept
{
    mGraphicsItem->setText(mNetSignal->getName());
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
