/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "si_symbol.h"

#include "../../../graphics/graphicsscene.h"
#include "../../../library/cmp/component.h"
#include "../../../library/sym/symbol.h"
#include "../../../utils/scopeguardlist.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "../../project.h"
#include "../../projectlibrary.h"
#include "../schematic.h"
#include "si_symbolpin.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_Symbol::SI_Symbol(Schematic& schematic, const Uuid& uuid,
                     ComponentInstance& cmpInstance, const Uuid& symbolItem,
                     const Point& position, const Angle& rotation,
                     bool mirrored)
  : SI_Base(schematic),
    mComponentInstance(cmpInstance),
    mSymbVarItem(mComponentInstance.getSymbolVariant()
                     .getSymbolItems()
                     .get(symbolItem)
                     .get()),  // can throw
    mSymbol(mSchematic.getProject().getLibrary().getSymbol(
        mSymbVarItem->getSymbolUuid())),
    mUuid(uuid),
    mPosition(position),
    mRotation(rotation),
    mMirrored(mirrored) {
  Q_ASSERT(mSymbVarItem);
  if (!mSymbol) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("No symbol with the UUID \"%1\" found in the "
                          "project's library.")
                           .arg(mSymbVarItem->getSymbolUuid().toStr()));
  }

  mGraphicsItem.reset(new SGI_Symbol(*this));
  mGraphicsItem->setPosition(mPosition);

  for (const SymbolPin& libPin : mSymbol->getPins()) {
    SI_SymbolPin* pin = new SI_SymbolPin(*this, libPin.getUuid());  // can throw
    if (mPins.contains(libPin.getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("The symbol pin UUID \"%1\" is defined multiple times.")
              .arg(libPin.getUuid().toStr()));
    }
    mPins.insert(libPin.getUuid(), pin);
  }
  if (mPins.count() != mSymbVarItem->getPinSignalMap().count()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("The pin count of the symbol instance \"%1\" "
                               "does not match with "
                               "the pin-signal-map of its component.")
                           .arg(mUuid.toStr()));
  }

  // connect to the "attributes changes" signal of schematic and component
  // instance
  connect(&mComponentInstance, &ComponentInstance::attributesChanged, this,
          [this]() { mGraphicsItem->updateAllTexts(); });
}

SI_Symbol::~SI_Symbol() noexcept {
  mGraphicsItem.reset();
  qDeleteAll(mPins);
  mPins.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString SI_Symbol::getName() const noexcept {
  if (mSymbVarItem->getSuffix()->isEmpty()) {
    return *mComponentInstance.getName();
  } else {
    return mComponentInstance.getName() % "-" % mSymbVarItem->getSuffix();
  }
}

QRectF SI_Symbol::getBoundingRect() const noexcept {
  return mGraphicsItem->sceneTransform().mapRect(mGraphicsItem->boundingRect());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_Symbol::setPosition(const Point& newPos) noexcept {
  if (newPos != mPosition) {
    mPosition = newPos;
    mGraphicsItem->setPosition(newPos);
    foreach (SI_SymbolPin* pin, mPins) { pin->updatePosition(false); }
  }
}

void SI_Symbol::setRotation(const Angle& newRotation) noexcept {
  if (newRotation != mRotation) {
    mRotation = newRotation;
    mGraphicsItem->updateRotationAndMirror();
    foreach (SI_SymbolPin* pin, mPins) { pin->updatePosition(true); }
  }
}

void SI_Symbol::setMirrored(bool newMirrored) noexcept {
  if (newMirrored != mMirrored) {
    mMirrored = newMirrored;
    mGraphicsItem->updateRotationAndMirror();
    foreach (SI_SymbolPin* pin, mPins) { pin->updatePosition(true); }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Symbol::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mPins.count() + 1);
  mComponentInstance.registerSymbol(*this);  // can throw
  sgl.add([&]() { mComponentInstance.unregisterSymbol(*this); });
  foreach (SI_SymbolPin* pin, mPins) {
    pin->addToSchematic();  // can throw
    sgl.add([pin]() { pin->removeFromSchematic(); });
  }
  SI_Base::addToSchematic(mGraphicsItem.data());
  sgl.dismiss();
}

void SI_Symbol::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mPins.count() + 1);
  foreach (SI_SymbolPin* pin, mPins) {
    pin->removeFromSchematic();  // can throw
    sgl.add([pin]() { pin->addToSchematic(); });
  }
  mComponentInstance.unregisterSymbol(*this);  // can throw
  sgl.add([&]() { mComponentInstance.registerSymbol(*this); });
  SI_Base::removeFromSchematic(mGraphicsItem.data());
  sgl.dismiss();
}

void SI_Symbol::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("component", mComponentInstance.getUuid());
  root.ensureLineBreak();
  root.appendChild("lib_gate", mSymbVarItem->getUuid());
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  root.appendChild("mirror", mMirrored);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString SI_Symbol::getBuiltInAttributeValue(const QString& key) const noexcept {
  if (key == QLatin1String("NAME")) {
    return getName();
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*> SI_Symbol::getAttributeProviderParents() const
    noexcept {
  return QVector<const AttributeProvider*>{&mSchematic, &mComponentInstance};
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_Symbol::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_Symbol::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
  mGraphicsItem->setSelected(selected);
  foreach (SI_SymbolPin* pin, mPins) { pin->setSelected(selected); }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SI_Symbol::checkAttributesValidity() const noexcept {
  if (mSymbVarItem == nullptr) return false;
  if (mSymbol == nullptr) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
