/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
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
class ComponentSignal final : public QObject, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentSignal() = delete;
  ComponentSignal(const ComponentSignal& other) noexcept;
  ComponentSignal(const Uuid& uuid, const CircuitIdentifier& name) noexcept;
  explicit ComponentSignal(const SExpression& node);
  ~ComponentSignal() noexcept;

  // Getters
  const Uuid&              getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  const SignalRole&        getRole() const noexcept { return mRole; }
  const QString& getForcedNetName() const noexcept { return mForcedNetName; }
  bool           isRequired() const noexcept { return mIsRequired; }
  bool           isNegated() const noexcept { return mIsNegated; }
  bool           isClock() const noexcept { return mIsClock; }
  bool           isNetSignalNameForced() const noexcept {
    return !mForcedNetName.isEmpty();
  }

  // Setters
  void setName(const CircuitIdentifier& name) noexcept;
  void setRole(const SignalRole& role) noexcept;
  void setForcedNetName(const QString& name) noexcept;
  void setIsRequired(bool required) noexcept;
  void setIsNegated(bool negated) noexcept;
  void setIsClock(bool clock) noexcept;

  // General Methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const ComponentSignal& rhs) const noexcept;
  bool operator!=(const ComponentSignal& rhs) const noexcept {
    return !(*this == rhs);
  }
  ComponentSignal& operator=(const ComponentSignal& rhs) noexcept;

signals:
  void edited();
  void nameChanged(const CircuitIdentifier& name);
  void roleChanged(const SignalRole& role);
  void forcedNetNameChanged(const QString& name);
  void isRequiredChanged(bool required);
  void isNegatedChanged(bool negated);
  void isClockChanged(bool clock);

private:  // Data
  Uuid              mUuid;
  CircuitIdentifier mName;
  SignalRole        mRole;
  QString           mForcedNetName;
  bool              mIsRequired;
  bool              mIsNegated;
  bool              mIsClock;
};

/*******************************************************************************
 *  Class ComponentSignalList
 ******************************************************************************/

struct ComponentSignalListNameProvider {
  static constexpr const char* tagname = "signal";
};
using ComponentSignalList =
    SerializableObjectList<ComponentSignal, ComponentSignalListNameProvider>;
using CmdComponentSignalInsert =
    CmdListElementInsert<ComponentSignal, ComponentSignalListNameProvider>;
using CmdComponentSignalRemove =
    CmdListElementRemove<ComponentSignal, ComponentSignalListNameProvider>;
using CmdComponentSignalsSwap =
    CmdListElementsSwap<ComponentSignal, ComponentSignalListNameProvider>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_COMPONENTSIGNAL_H
