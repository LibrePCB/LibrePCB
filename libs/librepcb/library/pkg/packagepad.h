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

#ifndef LIBREPCB_LIBRARY_PACKAGEPAD_H
#define LIBREPCB_LIBRARY_PACKAGEPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/circuitidentifier.h>
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include <librepcb/common/fileio/serializableobjectlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

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
class PackagePad final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(PackagePad)

public:
  // Signals
  enum class Event {
    UuidChanged,
    NameChanged,
  };
  Signal<PackagePad, Event>       onEdited;
  typedef Slot<PackagePad, Event> OnEditedSlot;

  // Constructors / Destructor
  PackagePad() = delete;
  PackagePad(const PackagePad& other) noexcept;
  PackagePad(const Uuid& uuid, const CircuitIdentifier& name) noexcept;
  explicit PackagePad(const SExpression& node);
  ~PackagePad() noexcept;

  // Getters
  const Uuid&       getUuid() const noexcept { return mUuid; }
  CircuitIdentifier getName() const noexcept { return mName; }

  // Setters
  bool setName(const CircuitIdentifier& name) noexcept;

  // General Methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const PackagePad& rhs) const noexcept;
  bool operator!=(const PackagePad& rhs) const noexcept {
    return !(*this == rhs);
  }
  PackagePad& operator=(const PackagePad& rhs) noexcept;

private:  // Data
  Uuid              mUuid;
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
using CmdPackagePadInsert =
    CmdListElementInsert<PackagePad, PackagePadListNameProvider,
                         PackagePad::Event>;
using CmdPackagePadRemove =
    CmdListElementRemove<PackagePad, PackagePadListNameProvider,
                         PackagePad::Event>;
using CmdPackagePadsSwap =
    CmdListElementsSwap<PackagePad, PackagePadListNameProvider,
                        PackagePad::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_PACKAGEPAD_H
