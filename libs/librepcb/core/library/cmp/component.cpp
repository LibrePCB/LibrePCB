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

#include "../../serialization/fileformatmigration.h"
#include "componentcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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

Component::Component(std::unique_ptr<TransactionalDirectory> directory,
                     const SExpression& root)
  : LibraryElement(getShortElementName(), getLongElementName(), true,
                   std::move(directory), root),
    mSchematicOnly(deserialize<bool>(root.getChild("schematic_only/@0"))),
    mDefaultValue(root.getChild("default_value/@0").getValue()),
    mPrefixes(root),
    mAttributes(root),
    mSignals(root),
    mSymbolVariants(root) {
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

RuleCheckMessageList Component::runChecks() const {
  ComponentCheck check(*this);
  return check.runChecks();  // can throw
}

std::unique_ptr<Component> Component::open(
    std::unique_ptr<TransactionalDirectory> directory,
    bool abortBeforeMigration) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  const auto migrations = FileFormatMigration::getMigrations(fileFormat);
  if (abortBeforeMigration && (!migrations.isEmpty())) {
    return nullptr;
  }
  for (auto migration : migrations) {
    migration->upgradeComponent(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const SExpression root = SExpression::parse(directory->read(fileName),
                                              directory->getAbsPath(fileName));
  std::unique_ptr<Component> obj(new Component(std::move(directory), root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
    obj->save();  // Format all files correctly as the migration doesn't!
  }
  return obj;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Component::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("schematic_only", mSchematicOnly);
  root.ensureLineBreak();
  root.appendChild("default_value", mDefaultValue);
  root.ensureLineBreak();
  mPrefixes.serialize(root);
  root.ensureLineBreak();
  mAttributes.serialize(root);
  root.ensureLineBreak();
  mSignals.serialize(root);
  root.ensureLineBreak();
  mSymbolVariants.serialize(root);
  root.ensureLineBreak();
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString Component::cleanNorm(QString norm) noexcept {
  return QString(norm.toUpper().remove(QRegularExpression("[^0-9A-Z]")));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
