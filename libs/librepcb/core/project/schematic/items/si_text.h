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

#ifndef LIBREPCB_CORE_SI_TEXT_H
#define LIBREPCB_CORE_SI_TEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/text.h"
#include "si_base.h"
#include "si_symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeProvider;
class LineGraphicsItem;
class Schematic;
class TextGraphicsItem;

/*******************************************************************************
 *  Class SI_Text
 ******************************************************************************/

/**
 * @brief The SI_Text class represents a text label in a schematic
 */
class SI_Text final : public SI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_Text() = delete;
  SI_Text(const SI_Text& other) = delete;
  SI_Text(Schematic& schematic, const Text& text);
  ~SI_Text() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mText.getUuid(); }
  const Point& getPosition() const noexcept { return mText.getPosition(); }
  const Angle& getRotation() const noexcept { return mText.getRotation(); }
  Text& getText() noexcept { return mText; }
  const Text& getText() const noexcept { return mText; }

  // General Methods
  SI_Symbol* getSymbol() const noexcept { return mSymbol; }
  void setSymbol(SI_Symbol* symbol) noexcept;
  const AttributeProvider* getAttributeProvider() const noexcept;
  void updateAnchor() noexcept;
  void addToSchematic() override;
  void removeFromSchematic() override;

  // Inherited from SI_Base
  Type_t getType() const noexcept override { return SI_Base::Type_t::Text; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  SI_Text& operator=(const SI_Text& rhs) = delete;

private:  // Methods
  void schematicOrSymbolAttributesChanged() noexcept;
  void textEdited(const Text& text, Text::Event event) noexcept;

private:  // Attributes
  QPointer<SI_Symbol> mSymbol;
  Text mText;
  QScopedPointer<TextGraphicsItem> mGraphicsItem;
  QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;

  // Slots
  Text::OnEditedSlot mOnTextEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
