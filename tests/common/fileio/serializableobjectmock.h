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

#ifndef SERIALIZABLEOBJECTMOCK_H
#define SERIALIZABLEOBJECTMOCK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <gmock/gmock.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/uuid.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Class MinimalSerializableObjectMock
 ****************************************************************************************/

class MinimalSerializableObjectMock final : public SerializableObject
{
    public:
        QString mValue;
        MinimalSerializableObjectMock() = delete;
        MinimalSerializableObjectMock(const QString& value) :
            mValue(value) {}
        MinimalSerializableObjectMock(const DomElement& root) :
            mValue(root.getText<QString>(false)) {}
        MinimalSerializableObjectMock(MinimalSerializableObjectMock&& other) = delete;
        MinimalSerializableObjectMock(const MinimalSerializableObjectMock& other) = delete;
        ~MinimalSerializableObjectMock() {}

        void serialize(DomElement& root) const throw (Exception) override {
            root.setText(mValue);
        }

        bool operator==(const MinimalSerializableObjectMock& rhs) = delete;
        bool operator!=(const MinimalSerializableObjectMock& rhs) = delete;
        MinimalSerializableObjectMock& operator=(const MinimalSerializableObjectMock& rhs) = delete;
};

/*****************************************************************************************
 *  Class SerializableObjectMock
 ****************************************************************************************/

class SerializableObjectMock final : public SerializableObject
{
    public:
        Uuid mUuid;
        QString mName;
        SerializableObjectMock() = delete;
        SerializableObjectMock(const SerializableObjectMock& other) noexcept :
            mUuid(other.mUuid), mName(other.mName) {}
        SerializableObjectMock(SerializableObjectMock&& other) noexcept :
            mUuid(std::move(other.mUuid)), mName(std::move(other.mName)) {}
        SerializableObjectMock(const Uuid& uuid, const QString& name) :
            mUuid(uuid), mName(name) {}
        SerializableObjectMock(const DomElement& root) :
            mUuid(root.getAttribute<Uuid>("uuid", true)), mName(root.getText<QString>(false)) {}
        ~SerializableObjectMock() {}

        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}

        void serialize(DomElement& root) const throw (Exception) override {
            root.setAttribute("uuid", mUuid);
            root.setText(mName);
        }

        bool operator==(const SerializableObjectMock& rhs) const noexcept {
            return (mUuid == rhs.mUuid) && (mName == rhs.mName);
        }
        bool operator!=(const SerializableObjectMock& rhs) const noexcept {
            return (mUuid != rhs.mUuid) || (mName != rhs.mName);
        }
        SerializableObjectMock& operator=(const SerializableObjectMock& rhs) noexcept {
            mUuid = rhs.mUuid;
            mName = rhs.mName;
            return *this;
        }
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb

#endif // SERIALIZABLEOBJECTMOCK_H

