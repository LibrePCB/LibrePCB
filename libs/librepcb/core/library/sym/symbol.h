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

#ifndef LIBREPCB_CORE_SYMBOL_H
#define LIBREPCB_CORE_SYMBOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/circle.h"
#include "../../geometry/polygon.h"
#include "../../geometry/text.h"
#include "../libraryelement.h"
#include "symbolpin.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SymbolGraphicsItem;

/*******************************************************************************
 *  Class Symbol
 ******************************************************************************/

/**
 * @brief The Symbol class represents the part of a component which is added to
 * schematics
 *
 * Following information is considered as the "interface" of a symbol and must
 * therefore never be changed:
 *  - UUID
 *  - Pins (neither adding nor removing pins is allowed)
 *    - UUID
 */
class Symbol final : public LibraryElement {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PinsEdited,
    PolygonsEdited,
    CirclesEdited,
    TextsEdited,
  };
  Signal<Symbol, Event> onEdited;
  typedef Slot<Symbol, Event> OnEditedSlot;

  // Constructors / Destructor
  Symbol() = delete;
  Symbol(const Symbol& other) = delete;
  Symbol(const Uuid& uuid, const Version& version, const QString& author,
         const ElementName& name_en_US, const QString& description_en_US,
         const QString& keywords_en_US);
  explicit Symbol(std::unique_ptr<TransactionalDirectory> directory);
  ~Symbol() noexcept;

  // Getters: Geometry
  SymbolPinList& getPins() noexcept { return mPins; }
  const SymbolPinList& getPins() const noexcept { return mPins; }
  PolygonList& getPolygons() noexcept { return mPolygons; }
  const PolygonList& getPolygons() const noexcept { return mPolygons; }
  CircleList& getCircles() noexcept { return mCircles; }
  const CircleList& getCircles() const noexcept { return mCircles; }
  TextList& getTexts() noexcept { return mTexts; }
  const TextList& getTexts() const noexcept { return mTexts; }

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const override;
  void registerGraphicsItem(SymbolGraphicsItem& item) noexcept;
  void unregisterGraphicsItem(SymbolGraphicsItem& item) noexcept;

  // Operator Overloadings
  Symbol& operator=(const Symbol& rhs) = delete;

  // Static Methods
  static QString getShortElementName() noexcept {
    return QStringLiteral("sym");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("symbol");
  }

private:  // Methods
  void pinsEdited(const SymbolPinList& list, int index,
                  const std::shared_ptr<const SymbolPin>& pin,
                  SymbolPinList::Event event) noexcept;
  void polygonsEdited(const PolygonList& list, int index,
                      const std::shared_ptr<const Polygon>& polygon,
                      PolygonList::Event event) noexcept;
  void circlesEdited(const CircleList& list, int index,
                     const std::shared_ptr<const Circle>& circle,
                     CircleList::Event event) noexcept;
  void textsEdited(const TextList& list, int index,
                   const std::shared_ptr<const Text>& text,
                   TextList::Event event) noexcept;
  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

private:  // Data
  SymbolPinList mPins;
  PolygonList mPolygons;
  CircleList mCircles;
  TextList mTexts;

  SymbolGraphicsItem* mRegisteredGraphicsItem;

  // Slots
  SymbolPinList::OnEditedSlot mPinsEditedSlot;
  PolygonList::OnEditedSlot mPolygonsEditedSlot;
  CircleList::OnEditedSlot mCirclesEditedSlot;
  TextList::OnEditedSlot mTextsEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
