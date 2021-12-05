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

#ifndef LIBREPCB_LIBRARY_COMPONENTSIGNAL_H
#define LIBREPCB_LIBRARY_COMPONENTSIGNAL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/circuitidentifier.h>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/signalrole.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class ComponentSignal
 ******************************************************************************/

/**
 * @brief The ComponentSignal class represents one signal of a component
 */
class ComponentSignal final : public SerializableObject {
public:
  // Signals
  enum class Event {
    UuidChanged,
    NameChanged,
    RoleChanged,
    ForcedNetNameChanged,
    IsRequiredChanged,
    IsNegatedChanged,
    IsClockChanged,
  };
  Signal<ComponentSignal, Event> onEdited;
  typedef Slot<ComponentSignal, Event> OnEditedSlot;

  // Constructors / Destructor
  ComponentSignal() = delete;
  ComponentSignal(const ComponentSignal& other) noexcept;
  ComponentSignal(const Uuid& uuid, const CircuitIdentifier& name,
                  const SignalRole& role, const QString& forcedNetName,
                  bool isRequired, bool isNegated, bool isClock) noexcept;
  ComponentSignal(const SExpression& node, const Version& fileFormat);
  ~ComponentSignal() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  const SignalRole& getRole() const noexcept { return mRole; }
  const QString& getForcedNetName() const noexcept { return mForcedNetName; }
  bool isRequired() const noexcept { return mIsRequired; }
  bool isNegated() const noexcept { return mIsNegated; }
  bool isClock() const noexcept { return mIsClock; }
  bool isNetSignalNameForced() const noexcept {
    return !mForcedNetName.isEmpty();
  }

  // Setters
  bool setName(const CircuitIdentifier& name) noexcept;
  bool setRole(const SignalRole& role) noexcept;
  bool setForcedNetName(const QString& name) noexcept;
  bool setIsRequired(bool required) noexcept;
  bool setIsNegated(bool negated) noexcept;
  bool setIsClock(bool clock) noexcept;

  // General Methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const ComponentSignal& rhs) const noexcept;
  bool operator!=(const ComponentSignal& rhs) const noexcept {
    return !(*this == rhs);
  }
  ComponentSignal& operator=(const ComponentSignal& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  CircuitIdentifier mName;
  SignalRole mRole;
  QString mForcedNetName;
  bool mIsRequired;
  bool mIsNegated;
  bool mIsClock;
};

/*******************************************************************************
 *  Class ComponentSignalList
 ******************************************************************************/

struct ComponentSignalListNameProvider {
  static constexpr const char* tagname = "signal";
};
using ComponentSignalList =
    SerializableObjectList<ComponentSignal, ComponentSignalListNameProvider,
                           ComponentSignal::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif
