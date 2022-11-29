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

#ifndef LIBREPCB_CORE_PACKAGEPAD_H
#define LIBREPCB_CORE_PACKAGEPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializableobjectlist.h"
#include "../../types/circuitidentifier.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PackagePad
 ******************************************************************************/

/**
 * @brief The PackagePad class represents one logical pad of a package
 *
 * Following information is considered as the "interface" of a pad and must
 * therefore never be changed:
 *  - UUID
 */
class PackagePad final {
  Q_DECLARE_TR_FUNCTIONS(PackagePad)

public:
  // Signals
  enum class Event {
    UuidChanged,
    NameChanged,
  };
  Signal<PackagePad, Event> onEdited;
  typedef Slot<PackagePad, Event> OnEditedSlot;

  // Constructors / Destructor
  PackagePad() = delete;
  PackagePad(const PackagePad& other) noexcept;
  PackagePad(const Uuid& uuid, const CircuitIdentifier& name) noexcept;
  PackagePad(const SExpression& node, const Version& fileFormat);
  ~PackagePad() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  CircuitIdentifier getName() const noexcept { return mName; }

  // Setters
  bool setName(const CircuitIdentifier& name) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const PackagePad& rhs) const noexcept;
  bool operator!=(const PackagePad& rhs) const noexcept {
    return !(*this == rhs);
  }
  PackagePad& operator=(const PackagePad& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  CircuitIdentifier mName;
};

/*******************************************************************************
 *  Class PackagePadList
 ******************************************************************************/

struct PackagePadListNameProvider {
  static constexpr const char* tagname = "pad";
};
using PackagePadList =
    SerializableObjectList<PackagePad, PackagePadListNameProvider,
                           PackagePad::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
