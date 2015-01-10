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
#include "symbolinstance.h"
#include "schematic.h"
#include "../project.h"
#include "../circuit/circuit.h"
#include "../../common/schematiclayer.h"
#include "../../library/symbolgraphicsitem.h"
#include "../library/projectlibrary.h"
#include "../circuit/gencompinstance.h"
#include "../../library/genericcomponent.h"
#include "../../library/symbol.h"
#include "symbolpininstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstance::SymbolInstance(Schematic& schematic, const QDomElement& domElement)
                               throw (Exception) :
    QObject(0), mSchematic(schematic), mDomElement(domElement), mSymbVarItem(0),
    mSymbol(0), mGraphicsItem(0), mGenCompInstance(0)
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

    mPosition.setXmm(mDomElement.firstChildElement("position").attribute("x"));
    mPosition.setYmm(mDomElement.firstChildElement("position").attribute("y"));
    mAngle.setAngleDeg(mDomElement.firstChildElement("position").attribute("angle"));

    QString symbVarItemUuid = mDomElement.attribute("symbol_item");
    mSymbVarItem = mGenCompInstance->getSymbolVariant().getItemByUuid(symbVarItemUuid);
    if (!mSymbVarItem)
    {
        throw RuntimeError(__FILE__, __LINE__, symbVarItemUuid,
            QString(tr("The symbol variant item UUID \"%1\" is invalid.")).arg(symbVarItemUuid));
    }

    mSymbol = mSchematic.getProject().getLibrary().getSymbol(mSymbVarItem->getSymbolUuid());
    if (!mSymbol)
    {
        throw RuntimeError(__FILE__, __LINE__, mSymbVarItem->getSymbolUuid().toString(),
            QString(tr("No symbol with the UUID \"%1\" found in the project's library."))
            .arg(mSymbVarItem->getSymbolUuid().toString()));
    }

    foreach (const library::SymbolPin* pin, mSymbol->getPins())
    {
        SymbolPinInstance* pinInstance = new SymbolPinInstance(*this, pin->getUuid());
        if (mPinInstances.contains(pin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pin->getUuid().toString(),
                QString(tr("The symbol pin UUID \"%1\" is defined multiple times."))
                .arg(pin->getUuid().toString()));
        }
        if (!mSymbVarItem->getPinSignalMap().contains(pin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pin->getUuid().toString(),
                QString(tr("Symbol pin UUID \"%1\" not found in pin-signal-map."))
                .arg(pin->getUuid().toString()));
        }
        mPinInstances.insert(pin->getUuid(), pinInstance);
    }
    if (mPinInstances.count() != mSymbVarItem->getPinSignalMap().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mPinInstances.count()).arg(mSymbVarItem->getPinSignalMap().count()),
            QString(tr("The pin count of the symbol instance \"%1\" does not match with "
            "the pin-signal-map")).arg(mUuid.toString()));
    }

    mGraphicsItem = new library::SymbolGraphicsItem(*mSymbol, this);
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(mAngle.toDeg());
}

SymbolInstance::~SymbolInstance() noexcept
{
    delete mGraphicsItem;           mGraphicsItem = 0;
    qDeleteAll(mPinInstances);      mPinInstances.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString SymbolInstance::getName() const noexcept
{
    return mGenCompInstance->getName() % mSymbVarItem->getSuffix();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SymbolInstance::setPosition(const Point& newPos) throw (Exception)
{
    mPosition = newPos;
    mGraphicsItem->setPos(newPos.toPxQPointF());
    foreach (SymbolPinInstance* pin, mPinInstances)
        pin->updateNetPointPosition();
}

void SymbolInstance::setAngle(const Angle& newAngle) throw (Exception)
{
    mAngle = newAngle;
    mGraphicsItem->setRotation(newAngle.toDeg());
    foreach (SymbolPinInstance* pin, mPinInstances)
        pin->updateNetPointPosition();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SymbolInstance::addToSchematic(Schematic& schematic, bool addNode,
                                       QDomElement& parent) throw (Exception)
{
    mGenCompInstance->registerSymbolInstance(mSymbVarItem->getUuid(), mSymbol->getUuid(), this);

    if (addNode)
    {
        if (parent.nodeName() != "symbols")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.appendChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
    }

    schematic.addItem(mGraphicsItem);

    foreach (SymbolPinInstance* pin, mPinInstances)
        pin->addToSchematic();
}

void SymbolInstance::removeFromSchematic(Schematic& schematic, bool removeNode,
                                            QDomElement& parent) throw (Exception)
{
    mGenCompInstance->unregisterSymbolInstance(mSymbVarItem->getUuid(), this);

    if (removeNode)
    {
        if (parent.nodeName() != "symbols")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.removeChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
    }

    schematic.removeItem(mGraphicsItem);

    foreach (SymbolPinInstance* pin, mPinInstances)
        pin->removeFromSchematic();
}

bool SymbolInstance::save(bool toOriginal, QStringList& errors) noexcept
{
    // save symbol attributes
    mDomElement.firstChildElement("position").setAttribute("x", mPosition.getX().toMmString());
    mDomElement.firstChildElement("position").setAttribute("y", mPosition.getY().toMmString());
    mDomElement.firstChildElement("position").setAttribute("angle", mAngle.toDegString());

    // save pin attributes
    bool success = true;
    foreach (SymbolPinInstance* pin, mPinInstances)
    {
        if (!pin->save(toOriginal, errors))
            success = false;
    }

    return success;
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

Point SymbolInstance::mapToScene(const Point& relativePos) const noexcept
{
    return (mPosition + relativePos).rotated(mAngle, mPosition);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

uint SymbolInstance::extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                              QList<SymbolInstance*>& symbols) noexcept
{
    foreach (QGraphicsItem* item, items)
    {
        Q_ASSERT(item); if (!item) continue;
        if (item->type() == CADScene::Type_Symbol)
        {
            library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(item);
            Q_ASSERT(i); if (!i) continue;
            SymbolInstance* s = i->getSymbolInstance();
            Q_ASSERT(s); if (!s) continue;
            if (!symbols.contains(s))
                symbols.append(s);
        }
    }
    return symbols.count();
}

SymbolInstance* SymbolInstance::create(Schematic& schematic, QDomDocument& doc,
                                       const QUuid& genCompInstance, const QUuid& symbolItem,
                                       const Point& position, const Angle& angle,
                                       bool mirror) throw (Exception)
{
    QDomElement node = doc.createElement("symbol");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("gen_comp_instance", genCompInstance.toString());
    node.setAttribute("symbol_item", symbolItem.toString());
    QDomElement posNode = doc.createElement("position");
    posNode.setAttribute("x", position.getX().toMmString());
    posNode.setAttribute("y", position.getY().toMmString());
    posNode.setAttribute("angle", angle.toDegString());
    posNode.setAttribute("mirror", mirror ? "true" : "false");
    node.appendChild(posNode);

    // create and return the new SymbolInstance object
    return new SymbolInstance(schematic, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
