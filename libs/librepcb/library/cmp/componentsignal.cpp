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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "componentsignal.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSignal::ComponentSignal(const ComponentSignal& other) noexcept :
    QObject(nullptr), mUuid(other.mUuid), mName(other.mName), mRole(other.mRole),
    mForcedNetName(other.mForcedNetName), mIsRequired(other.mIsRequired),
    mIsNegated(other.mIsNegated), mIsClock(other.mIsClock)
{
}

ComponentSignal::ComponentSignal(const Uuid& uuid, const QString& name) noexcept :
    QObject(nullptr), mUuid(uuid), mName(name), mRole(SignalRole::passive()),
    mForcedNetName(), mIsRequired(false), mIsNegated(false), mIsClock(false)
{
    Q_ASSERT(mUuid.isNull() == false);
}

ComponentSignal::ComponentSignal(const SExpression& node) :
    QObject(nullptr)
{
    // read attributes
    mUuid = node.getChildByIndex(0).getValue<Uuid>();
    mName = node.getValueByPath<QString>("name", true);
    mRole = node.getValueByPath<SignalRole>("role");
    mForcedNetName = node.getValueByPath<QString>("forced_net");
    mIsRequired = node.getValueByPath<bool>("required");
    mIsNegated = node.getValueByPath<bool>("negated");
    mIsClock = node.getValueByPath<bool>("clock");

    // backward compatibility - remove this some time!
    mForcedNetName.replace(QRegularExpression("#([_A-Za-z][_\\|0-9A-Za-z]*)"), "{{\\1}}");
    mForcedNetName.replace(QRegularExpression("\\{\\{(\\w+)\\|(\\w+)\\}\\}"), "{{ \\1 or \\2 }}");

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentSignal::~ComponentSignal() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentSignal::setName(const QString& name) noexcept
{
    if (name == mName) return;
    mName = name;
    emit nameChanged(mName);
    emit edited();
}

void ComponentSignal::setRole(const SignalRole& role) noexcept
{
    if (role == mRole) return;
    mRole = role;
    emit roleChanged(mRole);
    emit edited();
}

void ComponentSignal::setForcedNetName(const QString& name) noexcept
{
    if (name == mForcedNetName) return;
    mForcedNetName = name;
    emit forcedNetNameChanged(mForcedNetName);
    emit edited();
}

void ComponentSignal::setIsRequired(bool required) noexcept
{
    if (required == mIsRequired) return;
    mIsRequired = required;
    emit isRequiredChanged(mIsRequired);
    emit edited();
}

void ComponentSignal::setIsNegated(bool negated) noexcept
{
    if (negated == mIsNegated) return;
    mIsNegated = negated;
    emit isNegatedChanged(mIsNegated);
    emit edited();
}

void ComponentSignal::setIsClock(bool clock) noexcept
{
    if (clock == mIsClock) return;
    mIsClock = clock;
    emit isClockChanged(mIsClock);
    emit edited();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSignal::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("name", mName, false);
    root.appendChild("role", mRole, false);
    root.appendChild("required", mIsRequired, true);
    root.appendChild("negated", mIsNegated, false);
    root.appendChild("clock", mIsClock, false);
    root.appendChild("forced_net", mForcedNetName, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool ComponentSignal::operator==(const ComponentSignal& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                     return false;
    if (mName != rhs.mName)                     return false;
    if (mRole != rhs.mRole)                     return false;
    if (mForcedNetName != rhs.mForcedNetName)   return false;
    if (mIsRequired != rhs.mIsRequired)         return false;
    if (mIsNegated != rhs.mIsNegated)           return false;
    if (mIsClock != rhs.mIsClock)               return false;
    return true;
}

ComponentSignal& ComponentSignal::operator=(const ComponentSignal& rhs) noexcept
{
    if (mUuid != rhs.mUuid) {
        mUuid = rhs.mUuid;
        emit edited();
    }
    setName(rhs.mName);
    setRole(rhs.mRole);
    setForcedNetName(rhs.mForcedNetName);
    setIsRequired(rhs.mIsRequired);
    setIsNegated(rhs.mIsNegated);
    setIsClock(rhs.mIsClock);
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSignal::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
