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

#ifndef UNITTESTS_CORE_SERIALIZABLEOBJECTMOCK_H
#define UNITTESTS_CORE_SERIALIZABLEOBJECTMOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gmock/gmock.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Class MinimalSerializableObjectMock
 ******************************************************************************/

class MinimalSerializableObjectMock final {
public:
  QString mValue;
  Signal<MinimalSerializableObjectMock> onEdited;
  MinimalSerializableObjectMock() = delete;
  MinimalSerializableObjectMock(const QString& value)
    : mValue(value), onEdited(*this) {}
  explicit MinimalSerializableObjectMock(const SExpression& root)
    : mValue(root.getChild("@0").getValue()), onEdited(*this) {}
  MinimalSerializableObjectMock(MinimalSerializableObjectMock&& other) = delete;
  MinimalSerializableObjectMock(const MinimalSerializableObjectMock& other) =
      delete;
  ~MinimalSerializableObjectMock() {}

  void serialize(SExpression& root) const {
    root.ensureLineBreak();
    root.appendChild("value", mValue);
    root.ensureLineBreak();
  }

  bool operator==(const MinimalSerializableObjectMock& rhs) = delete;
  bool operator!=(const MinimalSerializableObjectMock& rhs) = delete;
  MinimalSerializableObjectMock& operator=(
      const MinimalSerializableObjectMock& rhs) = delete;
};

/*******************************************************************************
 *  Class SerializableObjectMock
 ******************************************************************************/

class SerializableObjectMock final {
public:
  Uuid mUuid;
  QString mName;
  Signal<SerializableObjectMock> onEdited;
  SerializableObjectMock() = delete;
  SerializableObjectMock(const SerializableObjectMock& other) noexcept
    : mUuid(other.mUuid), mName(other.mName), onEdited(*this) {}
  SerializableObjectMock(SerializableObjectMock&& other) noexcept
    : mUuid(std::move(other.mUuid)),
      mName(std::move(other.mName)),
      onEdited(*this) {}
  SerializableObjectMock(const Uuid& uuid, const QString& name)
    : mUuid(uuid), mName(name), onEdited(*this) {}
  explicit SerializableObjectMock(const SExpression& root)
    : mUuid(deserialize<Uuid>(root.getChild("@0"))),
      mName(root.getChild("name/@0").getValue()),
      onEdited(*this) {}
  ~SerializableObjectMock() {}

  const Uuid& getUuid() const noexcept { return mUuid; }
  const QString& getName() const noexcept { return mName; }

  void serialize(SExpression& root) const {
    root.appendChild(mUuid);
    root.ensureLineBreak();
    root.appendChild("name", mName);
    root.ensureLineBreak();
  }

  bool operator==(const SerializableObjectMock& rhs) const noexcept {
    return (mUuid == rhs.mUuid) && (mName == rhs.mName);
  }
  bool operator!=(const SerializableObjectMock& rhs) const noexcept {
    return (mUuid != rhs.mUuid) || (mName != rhs.mName);
  }
  SerializableObjectMock& operator=(
      const SerializableObjectMock& rhs) noexcept {
    mUuid = rhs.mUuid;
    mName = rhs.mName;
    return *this;
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

#endif
