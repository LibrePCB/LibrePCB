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
#include "../../common/file_io/xmldomelement.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstance::SymbolInstance(Schematic& schematic, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), IF_AttributeProvider(), mSchematic(schematic), mSymbVarItem(nullptr),
    mSymbol(nullptr), mGraphicsItem(nullptr), mGenCompInstance(nullptr)
{
    mUuid = domElement.getAttribute<QUuid>("uuid");

    QUuid gcUuid = domElement.getAttribute<QUuid>("gen_comp_instance");
    mGenCompInstance = schematic.getProject().getCircuit().getGenCompInstanceByUuid(gcUuid);
    if (!mGenCompInstance)
    {
        throw RuntimeError(__FILE__, __LINE__, gcUuid.toString(),
            QString(tr("No generic component with the UUID \"%1\" found in the circuit!"))
            .arg(gcUuid.toString()));
    }
    connect(mGenCompInstance, SIGNAL(attributesChanged()), this, SLOT(genCompAttributesChanged()));

    mPosition.setX(domElement.getFirstChild("position", true)->getAttribute<Length>("x"));
    mPosition.setY(domElement.getFirstChild("position", true)->getAttribute<Length>("y"));
    mAngle = domElement.getFirstChild("position", true)->getAttribute<Angle>("angle");

    QUuid symbVarItemUuid = domElement.getAttribute<QUuid>("symbol_item");
    init(symbVarItemUuid);
}

SymbolInstance::SymbolInstance(Schematic& schematic, GenCompInstance& genCompInstance,
                               const QUuid& symbolItem, const Point& position,
                               const Angle& angle) throw (Exception) :
    QObject(nullptr), IF_AttributeProvider(), mSchematic(schematic), mSymbVarItem(nullptr),
    mSymbol(nullptr), mGraphicsItem(nullptr), mGenCompInstance(&genCompInstance),
    mPosition(position), mAngle(angle)
{
    mUuid = QUuid::createUuid().toString(); // generate random UUID

    connect(mGenCompInstance, SIGNAL(attributesChanged()), this, SLOT(genCompAttributesChanged()));

    init(symbolItem);
}

void SymbolInstance::init(const QUuid& symbVarItemUuid) throw (Exception)
{
    mSymbVarItem = mGenCompInstance->getSymbolVariant().getItemByUuid(symbVarItemUuid);
    if (!mSymbVarItem)
    {
        throw RuntimeError(__FILE__, __LINE__, symbVarItemUuid.toString(),
            QString(tr("The symbol variant item UUID \"%1\" is invalid."))
            .arg(symbVarItemUuid.toString()));
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

void SymbolInstance::addToSchematic() throw (Exception)
{
    mGenCompInstance->registerSymbolInstance(mSymbVarItem->getUuid(), mSymbol->getUuid(), this);
    mSchematic.addItem(mGraphicsItem);
    foreach (SymbolPinInstance* pin, mPinInstances)
        pin->addToSchematic();
}

void SymbolInstance::removeFromSchematic() throw (Exception)
{
    mGenCompInstance->unregisterSymbolInstance(mSymbVarItem->getUuid(), this);
    mSchematic.removeItem(mGraphicsItem);
    foreach (SymbolPinInstance* pin, mPinInstances)
        pin->removeFromSchematic();
}

XmlDomElement* SymbolInstance::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(new XmlDomElement("symbol"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("gen_comp_instance", mGenCompInstance->getUuid());
    root->setAttribute("symbol_item", mSymbVarItem->getUuid());
    XmlDomElement* position = root->appendChild("position");
    position->setAttribute("x", mPosition.getX());
    position->setAttribute("y", mPosition.getY());
    position->setAttribute("angle", mAngle);
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

Point SymbolInstance::mapToScene(const Point& relativePos) const noexcept
{
    return (mPosition + relativePos).rotated(mAngle, mPosition);
}

bool SymbolInstance::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                       bool passToParents, QString& value) const noexcept
{
    if ((attrNS == QLatin1String("SYM")) || (attrNS.isEmpty()))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = getName(), true;
    }

    if ((attrNS != QLatin1String("SYM")) && (passToParents))
    {
        if (mGenCompInstance->getAttributeValue(attrNS, attrKey, false, value))
            return true;
        if (mSchematic.getAttributeValue(attrNS, attrKey, true, value))
            return true;
    }

    return false;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void SymbolInstance::genCompAttributesChanged()
{
    if (mGraphicsItem)
        mGraphicsItem->update();
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
