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

#include "../../../library/cmp/component.h"
#include "../../../library/sym/symbol.h"
#include "../../../utils/scopeguardlist.h"
#include "../../../utils/transform.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "../../project.h"
#include "../../projectlibrary.h"
#include "../schematic.h"
#include "si_symbolpin.h"
#include "si_text.h"

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
                     bool mirrored, bool loadInitialTexts)
  : SI_Base(schematic),
    onEdited(*this),
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

  // Add initial texts.
  if (loadInitialTexts) {
    for (const Text& text : getDefaultTexts()) {
      addText(*new SI_Text(mSchematic, text));
    }
  }

  if (mSymbol->getPins().count() != mSymbVarItem->getPinSignalMap().count()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The pin count of the symbol instance \"%1\" "
                "does not match with the pin-signal-map of its component.")
            .arg(mUuid.toStr()));
  }
  for (const SymbolPin& libPin : mSymbol->getPins()) {
    const ComponentPinSignalMapItem& item =
        *mSymbVarItem->getPinSignalMap().get(libPin.getUuid());  // can throw
    if (!item.getSignalUuid()) continue;  // Hide pins which are not connected.
    if (mPins.contains(libPin.getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("The symbol pin UUID \"%1\" is defined multiple times.")
              .arg(libPin.getUuid().toStr()));
    }
    SI_SymbolPin* pin = new SI_SymbolPin(*this, libPin, item);  // can throw
    mPins.insert(libPin.getUuid(), pin);
  }

  // Emit the "attributesChanged" signal when the schematic or component
  // instance has emitted it.
  connect(&mSchematic, &Schematic::attributesChanged, this,
          &SI_Symbol::attributesChanged);
  connect(&mComponentInstance, &ComponentInstance::attributesChanged, this,
          &SI_Symbol::attributesChanged);
}

SI_Symbol::~SI_Symbol() noexcept {
  qDeleteAll(mTexts);
  mTexts.clear();
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

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_Symbol::setPosition(const Point& newPos) noexcept {
  if (newPos != mPosition) {
    mPosition = newPos;
    onEdited.notify(Event::PositionChanged);
  }
}

void SI_Symbol::setRotation(const Angle& newRotation) noexcept {
  if (newRotation != mRotation) {
    mRotation = newRotation;
    onEdited.notify(Event::RotationChanged);
  }
}

void SI_Symbol::setMirrored(bool newMirrored) noexcept {
  if (newMirrored != mMirrored) {
    mMirrored = newMirrored;
    onEdited.notify(Event::MirroredChanged);
  }
}

/*******************************************************************************
 *  Text Methods
 ******************************************************************************/

TextList SI_Symbol::getDefaultTexts() const noexcept {
  // Copy all symbol texts and transform them to the global coordinate system
  // (not relative to the symbol). The original UUIDs are kept for future
  // identification.
  TextList texts = mSymbol->getTexts();
  Transform transform(*this);
  for (Text& text : texts) {
    text.setPosition(transform.map(text.getPosition()));
    text.setRotation(transform.mapNonMirrorable(text.getRotation()));
    if (transform.getMirrored()) {
      text.setAlign(text.getAlign().mirroredV());
    }
  }
  return texts;
}

void SI_Symbol::addText(SI_Text& text) {
  if ((mTexts.values().contains(&text)) ||
      (&text.getSchematic() != &mSchematic)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mTexts.contains(text.getUuid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("There is already a text with the UUID \"%1\"!")
                           .arg(text.getUuid().toStr()));
  }
  text.setSymbol(this);
  if (isAddedToSchematic()) {
    text.addToSchematic();  // can throw
  }
  mTexts.insert(text.getUuid(), &text);
  emit textAdded(text);
}

void SI_Symbol::removeText(SI_Text& text) {
  if (mTexts.value(text.getUuid()) != &text) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isAddedToSchematic()) {
    text.removeFromSchematic();  // can throw
  }
  mTexts.remove(text.getUuid());
  emit textRemoved(text);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Symbol::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mPins.count() + mTexts.count() + 1);
  mComponentInstance.registerSymbol(*this);  // can throw
  sgl.add([&]() { mComponentInstance.unregisterSymbol(*this); });
  foreach (SI_SymbolPin* pin, mPins) {
    pin->addToSchematic();  // can throw
    sgl.add([pin]() { pin->removeFromSchematic(); });
  }
  foreach (SI_Text* text, mTexts) {
    text->addToSchematic();  // can throw
    sgl.add([text]() { text->removeFromSchematic(); });
  }
  SI_Base::addToSchematic();
  sgl.dismiss();
}

void SI_Symbol::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mPins.count() + mTexts.count() + 1);
  foreach (SI_SymbolPin* pin, mPins) {
    pin->removeFromSchematic();  // can throw
    sgl.add([pin]() { pin->addToSchematic(); });
  }
  foreach (SI_Text* text, mTexts) {
    text->removeFromSchematic();  // can throw
    sgl.add([text]() { text->addToSchematic(); });
  }
  mComponentInstance.unregisterSymbol(*this);  // can throw
  sgl.add([&]() { mComponentInstance.registerSymbol(*this); });
  SI_Base::removeFromSchematic();
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
  for (const SI_Text* obj : mTexts) {
    root.ensureLineBreak();
    obj->getTextObj().serialize(root.appendList("text"));
  }
  root.ensureLineBreak();
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
