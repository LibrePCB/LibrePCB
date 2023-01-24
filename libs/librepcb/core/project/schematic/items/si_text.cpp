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

#include "../../../graphics/graphicsscene.h"
#include "../../../graphics/linegraphicsitem.h"
#include "../../../graphics/origincrossgraphicsitem.h"
#include "../../../graphics/textgraphicsitem.h"
#include "../../project.h"
#include "../schematic.h"
#include "../schematiclayerprovider.h"

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
    mSymbol(nullptr),
    mText(text),
    mGraphicsItem(
        new TextGraphicsItem(mText, mSchematic.getProject().getLayers())),
    mAnchorGraphicsItem(new LineGraphicsItem()),
    mOnTextEditedSlot(*this, &SI_Text::textEdited) {
  mText.onEdited.attach(mOnTextEditedSlot);

  mGraphicsItem->setZValue(Schematic::ZValue_Texts);
  mGraphicsItem->setAttributeProvider(&mSchematic);

  mAnchorGraphicsItem->setZValue(Schematic::ZValue_TextAnchors);

  // Connect to the "attributes changed" signal of the schematic.
  connect(&mSchematic, &Schematic::attributesChanged, this,
          &SI_Text::schematicOrSymbolAttributesChanged);
}

SI_Text::~SI_Text() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Text::setSymbol(SI_Symbol* symbol) noexcept {
  if (mSymbol) {
    disconnect(mSymbol, &SI_Symbol::attributesChanged, this,
               &SI_Text::schematicOrSymbolAttributesChanged);
  }

  mSymbol = symbol;
  mGraphicsItem->setAttributeProvider(getAttributeProvider());
  updateAnchor();

  // Text might need to be updated if symbol attributes have changed.
  if (mSymbol) {
    connect(mSymbol, &SI_Symbol::attributesChanged, this,
            &SI_Text::schematicOrSymbolAttributesChanged);
  }
}

const AttributeProvider* SI_Text::getAttributeProvider() const noexcept {
  if (mSymbol) {
    return mSymbol.data();
  } else {
    return &mSchematic;
  }
}

void SI_Text::updateAnchor() noexcept {
  if (mSymbol && isSelected()) {
    mAnchorGraphicsItem->setLine(mText.getPosition(), mSymbol->getPosition());
    mAnchorGraphicsItem->setLayer(
        mSchematic.getProject().getLayers().getLayer(*mText.getLayerName()));
  } else {
    mAnchorGraphicsItem->setLayer(nullptr);
  }
}

void SI_Text::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::addToSchematic(mGraphicsItem.data());
  mSchematic.getGraphicsScene().addItem(*mAnchorGraphicsItem);
}

void SI_Text::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::removeFromSchematic(mGraphicsItem.data());
  mSchematic.getGraphicsScene().removeItem(*mAnchorGraphicsItem);
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_Text::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_Text::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
  mGraphicsItem->setSelected(selected);
  updateAnchor();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SI_Text::schematicOrSymbolAttributesChanged() noexcept {
  // Attribute changed -> graphics item needs to perform attribute substitution.
  mGraphicsItem->updateText();
}

void SI_Text::textEdited(const Text& text, Text::Event event) noexcept {
  Q_UNUSED(text);
  switch (event) {
    case Text::Event::LayerNameChanged:
    case Text::Event::PositionChanged:
      updateAnchor();
      break;
    default:
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
