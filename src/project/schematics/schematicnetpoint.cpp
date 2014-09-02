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
#include <QGraphicsEllipseItem>
#include "schematicnetpoint.h"
#include "schematic.h"
#include "schematicnetline.h"
#include "../project.h"
#include "../circuit/circuit.h"
#include "../circuit/netsignal.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicNetPoint::SchematicNetPoint(Schematic& schematic, const QDomElement& domElement)
                                     throw (Exception) :
    QObject(0), mSchematic(schematic), mDomElement(domElement), mItem(0), mNetSignal(0)
{
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }

    QString netSignalUuid = mDomElement.attribute("netsignal");
    mNetSignal = mSchematic.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if(!mNetSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, netSignalUuid,
            QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid));
    }

    mAttached = (mDomElement.firstChildElement("attached").text() == "true");

    mItem = new QGraphicsEllipseItem(-2, -2, 4, 4);
    mItem->setPen(QPen(Qt::darkGreen, 0));
    mItem->setBrush(QBrush(Qt::darkGreen, Qt::SolidPattern));

    if (mAttached)
    {
        // TODO
    }
    else
    {
        mPosition.setX(Length::fromMm(mDomElement.firstChildElement("position").attribute("x")));
        mPosition.setY(Length::fromMm(mDomElement.firstChildElement("position").attribute("y")));
        mItem->setPos(mPosition.toPxQPointF());
    }
}

SchematicNetPoint::~SchematicNetPoint() noexcept
{
    delete mItem;       mItem = 0;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicNetPoint::setPosition(const Point& position) noexcept
{
    mPosition = position;
    mDomElement.firstChildElement("position").setAttribute("x", mPosition.getX().toMmString());
    mDomElement.firstChildElement("position").setAttribute("y", mPosition.getY().toMmString());

    mItem->setPos(mPosition.toPxQPointF());

    foreach (SchematicNetLine* line, mLines)
        line->updateLine();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicNetPoint::registerNetLine(SchematicNetLine* netline) noexcept
{
    Q_CHECK_PTR(netline);
    Q_ASSERT(!mLines.contains(netline));
    mLines.append(netline);
    netline->updateLine();
}

void SchematicNetPoint::unregisterNetLine(SchematicNetLine* netline) noexcept
{
    Q_CHECK_PTR(netline);
    Q_ASSERT(mLines.contains(netline));
    mLines.removeOne(netline);
    netline->updateLine();
}

void SchematicNetPoint::addToSchematic(Schematic& schematic, bool addNode,
                                       QDomElement& parent) throw (Exception)
{
    if (addNode)
    {
        if (parent.nodeName() != "netpoints")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.appendChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
    }

    mNetSignal->registerSchematicNetPoint(this);
    schematic.addItem(mItem);
}

void SchematicNetPoint::removeFromSchematic(Schematic& schematic, bool removeNode,
                                            QDomElement& parent) throw (Exception)
{
    if (removeNode)
    {
        if (parent.nodeName() != "netpoints")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.removeChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
    }

    schematic.removeItem(mItem);
    mNetSignal->unregisterSchematicNetPoint(this);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SchematicNetPoint* SchematicNetPoint::create(Schematic& schematic, QDomDocument& doc,
                                             const QUuid& netsignal, const Point& position) throw (Exception)
{
    QDomElement node = doc.createElement("netpoint");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("netsignal", netsignal.toString());
    QDomElement posNode = doc.createElement("position");
    posNode.setAttribute("x", position.getX().toMmString());
    posNode.setAttribute("y", position.getY().toMmString());
    node.appendChild(posNode);

    // create and return the new SchematicNetPoint object
    return new SchematicNetPoint(schematic, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
