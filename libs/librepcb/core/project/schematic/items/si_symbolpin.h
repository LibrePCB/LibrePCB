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

#ifndef LIBREPCB_CORE_SI_SYMBOLPIN_H
#define LIBREPCB_CORE_SI_SYMBOLPIN_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../types/angle.h"
#include "../../../types/point.h"
#include "si_base.h"
#include "si_netline.h"
#include "si_symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class ComponentPinSignalMapItem;
class ComponentSignal;
class ComponentSignalInstance;
class SymbolPin;

/*******************************************************************************
 *  Class SI_SymbolPin
 ******************************************************************************/

/**
 * @brief The SI_SymbolPin class
 */
class SI_SymbolPin final : public SI_Base, public SI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    RotationChanged,
    JunctionChanged,
    NameChanged,
    NumbersChanged,
    NumbersPositionChanged,
    NumbersAlignmentChanged,
    NetNameChanged,
  };
  Signal<SI_SymbolPin, Event> onEdited;
  typedef Slot<SI_SymbolPin, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_SymbolPin() = delete;
  SI_SymbolPin(const SI_SymbolPin& other) = delete;
  explicit SI_SymbolPin(SI_Symbol& symbol, const Uuid& pinUuid);
  ~SI_SymbolPin();

  // Getters

  /**
   * @brief Get the absolute position of the pin (scene coordinates)
   *
   * @return Absolute pin position
   */
  const Point& getPosition() const noexcept override { return mPosition; }

  /**
   * @brief Get the absolute rotation of the pin (scene coordinates)
   *
   * @return Absolute pin rotation
   */
  const Angle& getRotation() const noexcept { return mRotation; }

  const QString& getName() const noexcept { return mName; }

  const QStringList& getNumbers() const noexcept { return mNumbers; }

  const QString& getNumbersTruncated() const noexcept {
    return mNumbersTruncated;
  }

  const Point& getNumbersPosition() const noexcept { return mNumbersPosition; }

  const Alignment& getNumbersAlignment() const noexcept {
    return mNumbersAlignment;
  }

  const Uuid& getLibPinUuid() const noexcept;
  SI_Symbol& getSymbol() const noexcept { return mSymbol; }
  const SymbolPin& getLibPin() const noexcept { return *mSymbolPin; }
  ComponentSignalInstance* getComponentSignalInstance() const noexcept {
    return mComponentSignalInstance;
  }
  NetSignal* getCompSigInstNetSignal() const noexcept;
  SI_NetSegment* getNetSegmentOfLines() const noexcept;
  bool isRequired() const noexcept;
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  bool isVisibleJunction() const noexcept;
  bool isOpen() const noexcept override {
    return mRegisteredNetLines.isEmpty();
  }
  NetLineAnchor toNetLineAnchor() const noexcept override;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  // Inherited from SI_NetLineAnchor
  void registerNetLine(SI_NetLine& netline) override;
  void unregisterNetLine(SI_NetLine& netline) override;
  const QSet<SI_NetLine*>& getNetLines() const noexcept override {
    return mRegisteredNetLines;
  }

  // Operator Overloadings
  SI_SymbolPin& operator=(const SI_SymbolPin& rhs) = delete;

private:
  void symbolEdited(const SI_Symbol& obj, SI_Symbol::Event event) noexcept;
  void netSignalChanged(NetSignal* from, NetSignal* to) noexcept;
  void netSignalNameChanged() noexcept;
  void padNamesChanged(const QStringList& names) noexcept;
  void updateTransform() noexcept;
  void updateName() noexcept;
  void updateNumbers() noexcept;
  void updateNumbersTransform() noexcept;
  QString getLibraryComponentName() const noexcept;
  QString getComponentSignalNameOrPinUuid() const noexcept;
  QString getNetSignalName() const noexcept;

  // General
  SI_Symbol& mSymbol;
  const SymbolPin* mSymbolPin;
  const ComponentPinSignalMapItem* mPinSignalMapItem;
  ComponentSignalInstance* mComponentSignalInstance;

  // Cached Properties
  Point mPosition;
  Angle mRotation;
  QString mName;
  QStringList mNumbers;
  QString mNumbersTruncated;
  Point mNumbersPosition;
  Alignment mNumbersAlignment;

  // Registered Elements
  QSet<SI_NetLine*> mRegisteredNetLines;  ///< all registered netlines

  // Slots
  SI_Symbol::OnEditedSlot mOnSymbolEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
