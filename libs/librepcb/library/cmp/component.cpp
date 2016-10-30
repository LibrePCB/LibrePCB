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
#include "component.h"
#include <librepcb/common/fileio/domdocument.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Component::Component(const Uuid& uuid, const Version& version, const QString& author,
                     const QString& name_en_US, const QString& description_en_US,
                     const QString& keywords_en_US) :
    LibraryElement(getShortElementName(), getLongElementName(), uuid, version, author,
                   name_en_US, description_en_US, keywords_en_US),
    mSchematicOnly(false), mDefaultValue()
{
}

Component::Component(const FilePath& elementDirectory, bool readOnly) :
    LibraryElement(elementDirectory, getShortElementName(), getLongElementName(), readOnly),
    mSchematicOnly(false), mDefaultValue()
{
    DomElement& root = mLoadingXmlFileDocument->getRoot();

    // Load all properties
    mSchematicOnly = root.getFirstChild("schematic_only", true, true)->getText<bool>(true);
    mAttributes.loadFromDomElement(root); // can throw
    mDefaultValue = root.getFirstChild("default_value", true)->getText<QString>(false);
    mPrefixes.loadFromDomElement(root);
    mSignals.loadFromDomElement(root);
    mSymbolVariants.loadFromDomElement(root);

    cleanupAfterLoadingElementFromFile();
}

Component::~Component() noexcept
{
}

/*****************************************************************************************
 *  Convenience Methods
 ****************************************************************************************/

std::shared_ptr<ComponentSignal> Component::getSignalOfPin(
    const Uuid& symbVar, const Uuid& item, const Uuid& pin)
{
    Uuid sig = getSymbVarItem(symbVar, item)->getPinSignalMap().get(pin)->getSignalUuid(); // can throw
    if (sig.isNull()) return std::shared_ptr<ComponentSignal>(nullptr);
    return mSignals.get(sig); // can throw
}

std::shared_ptr<const ComponentSignal> Component::getSignalOfPin(const Uuid& symbVar,
    const Uuid& item, const Uuid& pin) const
{
    Uuid sig = getSymbVarItem(symbVar, item)->getPinSignalMap().get(pin)->getSignalUuid(); // can throw
    if (sig.isNull()) return std::shared_ptr<const ComponentSignal>(nullptr);
    return mSignals.get(sig); // can throw
}

std::shared_ptr<ComponentSymbolVariantItem> Component::getSymbVarItem(
    const Uuid& symbVar, const Uuid& item)
{
    return mSymbolVariants.get(symbVar)->getSymbolItems().get(item); // can throw
}

std::shared_ptr<const ComponentSymbolVariantItem> Component::getSymbVarItem(
    const Uuid& symbVar, const Uuid& item) const
{
    return mSymbolVariants.get(symbVar)->getSymbolItems().get(item); // can throw
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Component::serialize(DomElement& root) const
{
    LibraryElement::serialize(root);
    root.appendTextChild("schematic_only", mSchematicOnly);
    root.appendTextChild("default_value", mDefaultValue);
    mPrefixes.serialize(root);
    mAttributes.serialize(root);
    mSignals.serialize(root);
    mSymbolVariants.serialize(root);
}

bool Component::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())                 return false;
    if (!mPrefixes.contains(QString("")))                           return false;
    for (const ComponentSymbolVariant& var : mSymbolVariants) {
        for (const ComponentSymbolVariantItem& item : var.getSymbolItems()) {
            for (const ComponentPinSignalMapItem& map : item.getPinSignalMap()) {
                if (!map.getSignalUuid().isNull()) {
                    if (!mSignals.contains(map.getSignalUuid()))    return false;
                }
            }
        }
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
