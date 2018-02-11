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
#include "si_symbol.h"
#include "si_symbolpin.h"
#include "../schematic.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../library/projectlibrary.h"
#include "../../circuit/componentinstance.h"
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/scopeguardlist.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SI_Symbol::SI_Symbol(Schematic& schematic, const SExpression& node) :
    SI_Base(schematic), mComponentInstance(nullptr), mSymbVarItem(nullptr), mSymbol(nullptr)
{
    mUuid = node.getChildByIndex(0).getValue<Uuid>(true);
    Uuid gcUuid = node.getValueByPath<Uuid>("component", true);
    mComponentInstance = schematic.getProject().getCircuit().getComponentInstanceByUuid(gcUuid);
    if (!mComponentInstance) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("No component with the UUID \"%1\" found in the circuit!"))
            .arg(gcUuid.toStr()));
    }
    mPosition = Point(node.getChildByPath("pos"));
    mRotation = node.getValueByPath<Angle>("rot", true);
    Uuid symbVarItemUuid;
    if (node.tryGetChildByPath("lib_gate")) {
        symbVarItemUuid = node.getValueByPath<Uuid>("lib_gate", true);
    } else {
        // backward compatibility, remove this some time!
        symbVarItemUuid = node.getValueByPath<Uuid>("lib_item", true);
    }
    init(symbVarItemUuid);
}

SI_Symbol::SI_Symbol(Schematic& schematic, ComponentInstance& cmpInstance,
                     const Uuid& symbolItem, const Point& position, const Angle& rotation) :
    SI_Base(schematic), mComponentInstance(&cmpInstance), mSymbVarItem(nullptr),
    mSymbol(nullptr), mUuid(Uuid::createRandom()), mPosition(position), mRotation(rotation)
{
    init(symbolItem);
}

void SI_Symbol::init(const Uuid& symbVarItemUuid)
{
    mSymbVarItem = mComponentInstance->getSymbolVariant().getSymbolItems().get(symbVarItemUuid).get(); // can throw
    mSymbol = mSchematic.getProject().getLibrary().getSymbol(mSymbVarItem->getSymbolUuid());
    if (!mSymbol) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("No symbol with the UUID \"%1\" found in the project's library."))
            .arg(mSymbVarItem->getSymbolUuid().toStr()));
    }

    mGraphicsItem.reset(new SGI_Symbol(*this));
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    mGraphicsItem->setRotation(-mRotation.toDeg());

    for (const library::SymbolPin& libPin : mSymbol->getPins()) {
        SI_SymbolPin* pin = new SI_SymbolPin(*this, libPin.getUuid()); // can throw
        if (mPins.contains(libPin.getUuid())) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The symbol pin UUID \"%1\" is defined multiple times."))
                .arg(libPin.getUuid().toStr()));
        }
        mPins.insert(libPin.getUuid(), pin);
    }
    if (mPins.count() != mSymbVarItem->getPinSignalMap().count()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The pin count of the symbol instance \"%1\" does not match with "
            "the pin-signal-map of its component.")).arg(mUuid.toStr()));
    }

    // connect to the "attributes changes" signal of schematic and component instance
    connect(mComponentInstance, &ComponentInstance::attributesChanged,
            this, &SI_Symbol::schematicOrComponentAttributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SI_Symbol::~SI_Symbol() noexcept
{
    qDeleteAll(mPins);              mPins.clear();
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString SI_Symbol::getName() const noexcept
{
    return mComponentInstance->getName() % mSymbVarItem->getSuffix();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SI_Symbol::setPosition(const Point& newPos) noexcept
{
    if (newPos != mPosition) {
        mPosition = newPos;
        mGraphicsItem->setPos(newPos.toPxQPointF());
        mGraphicsItem->updateCacheAndRepaint();
        foreach (SI_SymbolPin* pin, mPins) {
            pin->updatePosition();
        }
    }
}

void SI_Symbol::setRotation(const Angle& newRotation) noexcept
{
    if (newRotation != mRotation) {
        mRotation = newRotation;
        mGraphicsItem->setRotation(-newRotation.toDeg());
        mGraphicsItem->updateCacheAndRepaint();
        foreach (SI_SymbolPin* pin, mPins) {
            pin->updatePosition();
        }
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SI_Symbol::addToSchematic()
{
    if (isAddedToSchematic()) {
        throw LogicError(__FILE__, __LINE__);
    }
    ScopeGuardList sgl(mPins.count()+1);
    mComponentInstance->registerSymbol(*this); // can throw
    sgl.add([&](){mComponentInstance->unregisterSymbol(*this);});
    foreach (SI_SymbolPin* pin, mPins) {
        pin->addToSchematic(); // can throw
        sgl.add([pin](){pin->removeFromSchematic();});
    }
    SI_Base::addToSchematic(mGraphicsItem.data());
    sgl.dismiss();
}

void SI_Symbol::removeFromSchematic()
{
    if (!isAddedToSchematic()) {
        throw LogicError(__FILE__, __LINE__);
    }
    ScopeGuardList sgl(mPins.count()+1);
    foreach (SI_SymbolPin* pin, mPins) {
        pin->removeFromSchematic(); // can throw
        sgl.add([pin](){pin->addToSchematic();});
    }
    mComponentInstance->unregisterSymbol(*this); // can throw
    sgl.add([&](){mComponentInstance->registerSymbol(*this);});
    SI_Base::removeFromSchematic(mGraphicsItem.data());
    sgl.dismiss();
}

void SI_Symbol::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendTokenChild("component", mComponentInstance->getUuid(), true);
    root.appendTokenChild("lib_gate", mSymbVarItem->getUuid(), true);
    root.appendChild(mPosition.serializeToDomElement("pos"), true);
    root.appendTokenChild("rot", mRotation, false);
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

Point SI_Symbol::mapToScene(const Point& relativePos) const noexcept
{
    return (mPosition + relativePos).rotated(mRotation, mPosition);
}

/*****************************************************************************************
 *  Inherited from AttributeProvider
 ****************************************************************************************/

QString SI_Symbol::getBuiltInAttributeValue(const QString& key) const noexcept
{
    if (key == QLatin1String("NAME")) {
        return getName();
    } else {
        return QString();
    }
}

QVector<const AttributeProvider*> SI_Symbol::getAttributeProviderParents() const noexcept
{
    return QVector<const AttributeProvider*>{&mSchematic, mComponentInstance};
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
    foreach (SI_SymbolPin* pin, mPins) {
        pin->setSelected(selected);
    }
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void SI_Symbol::schematicOrComponentAttributesChanged()
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
    if (mComponentInstance == nullptr)  return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
