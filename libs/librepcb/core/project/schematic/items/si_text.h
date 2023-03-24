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
#include "../../../utils/signalslot.h"
#include "si_base.h"
#include "si_symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeProvider;
class SI_Symbol;
class Schematic;

/*******************************************************************************
 *  Class SI_Text
 ******************************************************************************/

/**
 * @brief The SI_Text class represents a text label in a schematic
 */
class SI_Text final : public SI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    LayerNameChanged,
    TextChanged,
  };
  Signal<SI_Text, Event> onEdited;
  typedef Slot<SI_Text, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_Text() = delete;
  SI_Text(const SI_Text& other) = delete;
  SI_Text(Schematic& schematic, const Text& text);
  ~SI_Text() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mTextObj.getUuid(); }
  const Point& getPosition() const noexcept { return mTextObj.getPosition(); }
  const Angle& getRotation() const noexcept { return mTextObj.getRotation(); }
  const QString& getText() const noexcept { return mText; }
  Text& getTextObj() noexcept { return mTextObj; }
  const Text& getTextObj() const noexcept { return mTextObj; }

  // General Methods
  SI_Symbol* getSymbol() const noexcept { return mSymbol; }
  void setSymbol(SI_Symbol* symbol) noexcept;
  const AttributeProvider* getAttributeProvider() const noexcept;
  void addToSchematic() override;
  void removeFromSchematic() override;

  // Operator Overloadings
  SI_Text& operator=(const SI_Text& rhs) = delete;

private:  // Methods
  void textEdited(const Text& text, Text::Event event) noexcept;
  void updateText() noexcept;

private:  // Attributes
  QPointer<SI_Symbol> mSymbol;
  Text mTextObj;

  // Cached Attributes
  QString mText;

  // Slots
  Text::OnEditedSlot mOnTextEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
