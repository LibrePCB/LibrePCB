/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "si_symbol.h"
#include "si_symbolpin.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../library/projectlibrary.h"
#include "../../circuit/gencompinstance.h"
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/graphics/graphicsscene.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_Symbol::SI_Symbol(Schematic& schematic, const XmlDomElement& domElement) throw (Exception) :
    SI_Base(), mSchematic(schematic), mGenCompInstance(nullptr),
    mSymbVarItem(nullptr), mSymbol(nullptr), mGraphicsItem(nullptr)
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

    mPosition.setX(domElement.getFirstChild("position", true)->getAttribute<Length>("x"));
    mPosition.setY(domElement.getFirstChild("position", true)->getAttribute<Length>("y"));
    mAngle = domElement.getFirstChild("position", true)->getAttribute<Angle>("angle");

    QUuid symbVarItemUuid = domElement.getAttribute<QUuid>("symbol_item");
    init(symbVarItemUuid);
}

SI_Symbol::SI_Symbol(Schematic& schematic, GenCompInstance& genCompInstance,
                     const QUuid& symbolItem, const Point& position, const Angle& angle) throw (Exception) :
    SI_Base(), mSchematic(schematic), mGenCompInstance(&genCompInstance),
    mSymbVarItem(nullptr), mSymbol(nullptr), mGraphicsItem(nullptr), mPosition(position),
    mAngle(angle)
{
    mUuid = QUuid::createUuid().toString(); // generate random UUID
    init(symbolItem);
}

void SI_Symbol::init(const QUuid& symbVarItemUuid) throw (Exception)
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

    mGraphicsItem = new SGI_Symbol(*this);
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(mAngle.toDeg());

    foreach (const library::SymbolPin* libPin, mSymbol->getPins())
    {
        SI_SymbolPin* pin = new SI_SymbolPin(*this, libPin->getUuid());
        if (mPins.contains(libPin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, libPin->getUuid().toString(),
                QString(tr("The symbol pin UUID \"%1\" is defined multiple times."))
                .arg(libPin->getUuid().toString()));
        }
        if (!mSymbVarItem->getPinSignalMap().contains(libPin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, libPin->getUuid().toString(),
                QString(tr("Symbol pin UUID \"%1\" not found in pin-signal-map."))
                .arg(libPin->getUuid().toString()));
        }
        mPins.insert(libPin->getUuid(), pin);
    }
    if (mPins.count() != mSymbVarItem->getPinSignalMap().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mPins.count()).arg(mSymbVarItem->getPinSignalMap().count()),
            QString(tr("The pin count of the symbol instance \"%1\" does not match with "
            "the pin-signal-map")).arg(mUuid.toString()));
    }

    // connect to the "attributes changes" signal of schematic and generic component
    connect(mGenCompInstance, &GenCompInstance::attributesChanged,
            this, &SI_Symbol::schematicOrGenCompAttributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SI_Symbol::~SI_Symbol() noexcept
{
    qDeleteAll(mPins);              mPins.clear();
    delete mGraphicsItem;           mGraphicsItem = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Workspace& SI_Symbol::getWorkspace() const noexcept
{
    return mSchematic.getWorkspace();
}

Project& SI_Symbol::getProject() const noexcept
{
    return mSchematic.getProject();
}

QString SI_Symbol::getName() const noexcept
{
    return mGenCompInstance->getName() % mSymbVarItem->getSuffix();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_Symbol::setPosition(const Point& newPos) throw (Exception)
{
    mPosition = newPos;
    mGraphicsItem->setPos(newPos.toPxQPointF());
    mGraphicsItem->updateCacheAndRepaint();
    foreach (SI_SymbolPin* pin, mPins)
        pin->updatePosition();
}

void SI_Symbol::setAngle(const Angle& newAngle) throw (Exception)
{
    mAngle = newAngle;
    mGraphicsItem->setRotation(newAngle.toDeg());
    mGraphicsItem->updateCacheAndRepaint();
    foreach (SI_SymbolPin* pin, mPins)
        pin->updatePosition();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_Symbol::addToSchematic(GraphicsScene& scene) throw (Exception)
{
    mGenCompInstance->registerSymbol(*this);
    scene.addItem(*mGraphicsItem);
    foreach (SI_SymbolPin* pin, mPins)
        pin->addToSchematic(scene);
}

void SI_Symbol::removeFromSchematic(GraphicsScene& scene) throw (Exception)
{
    mGenCompInstance->unregisterSymbol(*this);
    scene.removeItem(*mGraphicsItem);
    foreach (SI_SymbolPin* pin, mPins)
        pin->removeFromSchematic(scene);
}

XmlDomElement* SI_Symbol::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

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

Point SI_Symbol::mapToScene(const Point& relativePos) const noexcept
{
    return (mPosition + relativePos).rotated(mAngle, mPosition);
}

bool SI_Symbol::getAttributeValue(const QString& attrNS, const QString& attrKey,
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
 *  Inherited from SI_Base
 ****************************************************************************************/

QPainterPath SI_Symbol::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_Symbol::setSelected(bool selected) noexcept
{
    SI_Base::setSelected(selected);
    mGraphicsItem->update();
    foreach (SI_SymbolPin* pin, mPins)
        pin->setSelected(selected);
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void SI_Symbol::schematicOrGenCompAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SI_Symbol::checkAttributesValidity() const noexcept
{
    if (mSymbVarItem == nullptr)        return false;
    if (mSymbol == nullptr)             return false;
    if (mUuid.isNull())                 return false;
    if (mGenCompInstance == nullptr)    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
