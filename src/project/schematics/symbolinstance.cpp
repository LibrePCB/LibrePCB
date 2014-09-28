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
#include <QGraphicsRectItem>
#include "symbolinstance.h"
#include "schematic.h"
#include "../project.h"
#include "../circuit/circuit.h"
#include "../../common/schematiclayer.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstance::SymbolInstance(Schematic& schematic, const QDomElement& domElement)
                               throw (Exception) :
    QObject(0), mSchematic(schematic), mDomElement(domElement), mItem(0), mOutlineLayer(0)
{
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid symbol instance UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }

    QString gcUuid = mDomElement.attribute("gen_comp_instance");
    mGenCompInstance = schematic.getProject().getCircuit().getGenCompInstanceByUuid(gcUuid);
    if (!mGenCompInstance)
    {
        throw RuntimeError(__FILE__, __LINE__, gcUuid,
            QString(tr("No generic component with the UUID \"%1\" found in the circuit!"))
                           .arg(gcUuid));
    }

    mOutlineLayer = mSchematic.getProject().getSchematicLayer(SchematicLayer::SymbolOutlines);
    if (!mOutlineLayer)
        throw LogicError(__FILE__, __LINE__, QString(), tr("No Outline Layer found!"));

    mItem = new QGraphicsRectItem(-10, -10, 20, 20);
    mItem->setZValue(Schematic::ZValue_Symbols);
    mItem->setPen(QPen(mOutlineLayer->getColor(), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    mItem->setBrush(QBrush(mOutlineLayer->getFillColor(), Qt::SolidPattern));

    mPosition.setX(Length::fromMm(mDomElement.firstChildElement("position").attribute("x")));
    mPosition.setY(Length::fromMm(mDomElement.firstChildElement("position").attribute("y")));
    mItem->setPos(mPosition.toPxQPointF());
}

SymbolInstance::~SymbolInstance() noexcept
{
    delete mItem;       mItem = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolInstance::addToSchematic(Schematic& schematic, bool addNode,
                                       QDomElement& parent) throw (Exception)
{
    if (addNode)
    {
        if (parent.nodeName() != "symbols")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.appendChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
    }

    schematic.addItem(mItem);
}

void SymbolInstance::removeFromSchematic(Schematic& schematic, bool removeNode,
                                            QDomElement& parent) throw (Exception)
{
    if (removeNode)
    {
        if (parent.nodeName() != "symbols")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.removeChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
    }

    schematic.removeItem(mItem);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SymbolInstance* SymbolInstance::create(Schematic& schematic, QDomDocument& doc,
                                       const QUuid& genCompInstance,
                                       const QUuid& symbolItem) throw (Exception)
{
    QDomElement node = doc.createElement("symbol");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("gen_comp_instance", genCompInstance.toString());
    node.setAttribute("symbol_item", symbolItem.toString());

    // create and return the new SymbolInstance object
    return new SymbolInstance(schematic, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
