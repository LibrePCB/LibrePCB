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
#include "si_text.h"

#include "../../../attribute/attributesubstitutor.h"
#include "../../project.h"
#include "../schematic.h"
#include "../schematiclayerprovider.h"
#include "si_symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_Text::SI_Text(Schematic& schematic, const Text& text)
  : SI_Base(schematic),
    onEdited(*this),
    mSymbol(nullptr),
    mTextObj(text),
    mText(),
    mOnTextEditedSlot(*this, &SI_Text::textEdited) {
  mTextObj.onEdited.attach(mOnTextEditedSlot);

  // Connect to the "attributes changed" signal of the schematic.
  connect(&mSchematic, &Schematic::attributesChanged, this,
          &SI_Text::updateText);

  updateText();
}

SI_Text::~SI_Text() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Text::setSymbol(SI_Symbol* symbol) noexcept {
  if (symbol == mSymbol) {
    return;
  }

  if (mSymbol) {
    disconnect(mSymbol, &SI_Symbol::attributesChanged, this,
               &SI_Text::updateText);
  }

  mSymbol = symbol;

  // Text might need to be updated if symbol attributes have changed.
  if (mSymbol) {
    connect(mSymbol, &SI_Symbol::attributesChanged, this, &SI_Text::updateText);
  }

  updateText();
}

const AttributeProvider* SI_Text::getAttributeProvider() const noexcept {
  if (mSymbol) {
    return mSymbol.data();
  } else {
    return &mSchematic;
  }
}

void SI_Text::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::addToSchematic();
}

void SI_Text::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::removeFromSchematic();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SI_Text::textEdited(const Text& text, Text::Event event) noexcept {
  Q_UNUSED(text);
  switch (event) {
    case Text::Event::PositionChanged: {
      onEdited.notify(Event::PositionChanged);
      break;
    }
    case Text::Event::LayerNameChanged: {
      onEdited.notify(Event::LayerNameChanged);
      break;
    }
    case Text::Event::TextChanged: {
      updateText();
      break;
    }
    default:
      break;
  }
}

void SI_Text::updateText() noexcept {
  const QString text = AttributeSubstitutor::substitute(mTextObj.getText(),
                                                        getAttributeProvider());
  if (text != mText) {
    mText = text;
    onEdited.notify(Event::TextChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
