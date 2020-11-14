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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "component.h"

#include "componentcheck.h"

#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Component::Component(const Uuid& uuid, const Version& version,
                     const QString& author, const ElementName& name_en_US,
                     const QString& description_en_US,
                     const QString& keywords_en_US)
  : LibraryElement(getShortElementName(), getLongElementName(), uuid, version,
                   author, name_en_US, description_en_US, keywords_en_US),
    mSchematicOnly(false),
    mDefaultValue(),
    mPrefixes(ComponentPrefix("")) {
}

Component::Component(std::unique_ptr<TransactionalDirectory> directory)
  : LibraryElement(std::move(directory), getShortElementName(),
                   getLongElementName()),
    mSchematicOnly(false),
    mDefaultValue(),
    mPrefixes(ComponentPrefix("")) {
  // Load all properties
  mSchematicOnly =
      deserialize<bool>(mLoadingFileDocument.getChild("schematic_only/@0"));
  mAttributes.loadFromSExpression(mLoadingFileDocument);  // can throw
  mDefaultValue = mLoadingFileDocument.getChild("default_value/@0").getValue();
  mPrefixes = NormDependentPrefixMap(mLoadingFileDocument);
  mSignals.loadFromSExpression(mLoadingFileDocument);
  mSymbolVariants.loadFromSExpression(mLoadingFileDocument);

  cleanupAfterLoadingElementFromFile();
}

Component::~Component() noexcept {
}

/*******************************************************************************
 *  Convenience Methods
 ******************************************************************************/

std::shared_ptr<ComponentSignal> Component::getSignalOfPin(const Uuid& symbVar,
                                                           const Uuid& item,
                                                           const Uuid& pin) {
  tl::optional<Uuid> sig = getSymbVarItem(symbVar, item)
                               ->getPinSignalMap()
                               .get(pin)
                               ->getSignalUuid();  // can throw
  if (sig) {
    return mSignals.get(*sig);  // can throw
  } else {
    return std::shared_ptr<ComponentSignal>(nullptr);
  }
}

std::shared_ptr<const ComponentSignal> Component::getSignalOfPin(
    const Uuid& symbVar, const Uuid& item, const Uuid& pin) const {
  tl::optional<Uuid> sig = getSymbVarItem(symbVar, item)
                               ->getPinSignalMap()
                               .get(pin)
                               ->getSignalUuid();  // can throw
  if (sig) {
    return mSignals.get(*sig);  // can throw
  } else {
    return std::shared_ptr<const ComponentSignal>(nullptr);
  }
}

int Component::getSymbolVariantIndexByNorm(const QStringList& normOrder) const
    noexcept {
  foreach (const QString& norm, normOrder) {
    for (int i = 0; i < mSymbolVariants.count(); ++i) {
      std::shared_ptr<const ComponentSymbolVariant> var = mSymbolVariants.at(i);
      if (cleanNorm(var->getNorm()) == cleanNorm(norm)) {
        return i;
      }
    }
  }
  return -1;
}

std::shared_ptr<ComponentSymbolVariantItem> Component::getSymbVarItem(
    const Uuid& symbVar, const Uuid& item) {
  return mSymbolVariants.get(symbVar)->getSymbolItems().get(item);  // can throw
}

std::shared_ptr<const ComponentSymbolVariantItem> Component::getSymbVarItem(
    const Uuid& symbVar, const Uuid& item) const {
  return mSymbolVariants.get(symbVar)->getSymbolItems().get(item);  // can throw
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList Component::runChecks() const {
  ComponentCheck check(*this);
  return check.runChecks();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Component::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.appendChild("schematic_only", mSchematicOnly, true);
  root.appendChild("default_value", mDefaultValue, true);
  mPrefixes.serialize(root);
  mAttributes.serialize(root);
  mSignals.serialize(root);
  mSymbolVariants.serialize(root);
}

QString Component::cleanNorm(QString norm) noexcept {
  return QString(norm.toUpper().remove(QRegularExpression("[^0-9A-Z]")));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
