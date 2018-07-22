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
#include "componentsymbolvariant.h"
#include "component.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSymbolVariant::ComponentSymbolVariant(const ComponentSymbolVariant& other) noexcept :
    QObject(nullptr), mSymbolItems(this)
{
    *this = other; // use assignment operator
}

ComponentSymbolVariant::ComponentSymbolVariant(const Uuid& uuid, const QString& norm,
                                               const QString& name_en_US,
                                               const QString& desc_en_US) noexcept :
    QObject(nullptr), mUuid(uuid), mNorm(norm), mSymbolItems(this)
{
    Q_ASSERT(!mUuid.isNull());
    mNames.setDefaultValue(name_en_US);
    mDescriptions.setDefaultValue(desc_en_US);
}

ComponentSymbolVariant::ComponentSymbolVariant(const SExpression& node) :
    QObject(nullptr), mSymbolItems(this)
{
    // read attributes
    mUuid = node.getChildByIndex(0).getValue<Uuid>();
    mNorm = node.getValueByPath<QString>("norm");
    mNames.loadFromDomElement(node);
    mDescriptions.loadFromDomElement(node);

    // Load all symbol variant items
    mSymbolItems.loadFromDomElement(node);

    // backward compatibility, remove this some time!
    foreach (const SExpression& node, node.getChildren("item")) {
        mSymbolItems.append(std::make_shared<ComponentSymbolVariantItem>(node)); // can throw
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentSymbolVariant::~ComponentSymbolVariant() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentSymbolVariant::setNorm(const QString& norm) noexcept
{
    mNorm = norm;
    emit edited();
}

void ComponentSymbolVariant::setName(const QString& locale, const QString& name) noexcept
{
    if (mNames.contains(locale) && mNames.value(locale) == name) return;
    mNames.insert(locale, name);
    emit edited();
}

void ComponentSymbolVariant::setDescription(const QString& locale, const QString& desc) noexcept
{
    if (mDescriptions.contains(locale) && mDescriptions.value(locale) == desc) return;
    mDescriptions.insert(locale, desc);
    emit edited();
}

void ComponentSymbolVariant::setNames(const LocalizedNameMap& names) noexcept
{
    if (names == mNames) return;
    mNames = names;
    emit edited();
}

void ComponentSymbolVariant::setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept
{
    if (descriptions == mDescriptions) return;
    mDescriptions = descriptions;
    emit edited();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSymbolVariant::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("norm", mNorm, false);
    mNames.serialize(root);
    mDescriptions.serialize(root);
    mSymbolItems.serialize(root);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool ComponentSymbolVariant::operator==(const ComponentSymbolVariant& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                                 return false;
    if (mNorm != rhs.mNorm)                                 return false;
    if (mNames != rhs.mNames)                               return false;
    if (mDescriptions != rhs.mDescriptions)                 return false;
    if (mSymbolItems != rhs.mSymbolItems)                   return false;
    return true;
}

ComponentSymbolVariant& ComponentSymbolVariant::operator=(const ComponentSymbolVariant& rhs) noexcept
{
    if (mUuid != rhs.mUuid) {
        mUuid = rhs.mUuid;
        emit edited();
    }
    setNorm(rhs.mNorm);
    setNames(rhs.mNames);
    setDescriptions(rhs.mDescriptions);
    if (mSymbolItems != rhs.mSymbolItems) {
        mSymbolItems = rhs.mSymbolItems;
        emit edited();
    }
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ComponentSymbolVariant::listObjectAdded(const ComponentSymbolVariantItemList& list,
    int newIndex, const std::shared_ptr<ComponentSymbolVariantItem>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(newIndex);
    Q_UNUSED(ptr);
    Q_ASSERT(&list == &mSymbolItems);
    emit edited();
}

void ComponentSymbolVariant::listObjectRemoved(const ComponentSymbolVariantItemList& list,
    int oldIndex, const std::shared_ptr<ComponentSymbolVariantItem>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(oldIndex);
    Q_UNUSED(ptr);
    Q_ASSERT(&list == &mSymbolItems);
    emit edited();
}

bool ComponentSymbolVariant::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mNames.getDefaultValue().isEmpty()) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
