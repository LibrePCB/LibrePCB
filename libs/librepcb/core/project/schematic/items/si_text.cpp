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
  : SI_Base(schematic), mText(text) {
  // Create the graphics item.
  mGraphicsItem.reset(
      new TextGraphicsItem(mText, mSchematic.getProject().getLayers()));
  mGraphicsItem->setZValue(Schematic::ZValue_Texts);
  mGraphicsItem->setAttributeProvider(&mSchematic);

  // Connect to the "attributes changed" signal of the schematic.
  connect(&mSchematic, &Schematic::attributesChanged, this,
          &SI_Text::schematicAttributesChanged);
}

SI_Text::~SI_Text() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Text::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::addToSchematic(mGraphicsItem.data());
}

void SI_Text::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::removeFromSchematic(mGraphicsItem.data());
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
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SI_Text::schematicAttributesChanged() noexcept {
  // Attribute changed -> graphics item needs to perform attribute substitution.
  mGraphicsItem->updateText();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
