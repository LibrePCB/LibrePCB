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

#ifndef LIBREPCB_CORE_COMPONENT_H
#define LIBREPCB_CORE_COMPONENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../attribute/attribute.h"
#include "../../serialization/serializablekeyvaluemap.h"
#include "../libraryelement.h"
#include "componentprefix.h"
#include "componentsignal.h"
#include "componentsymbolvariant.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class NormDependentPrefixMap
 ******************************************************************************/

struct NormDependentPrefixMapPolicy {
  typedef ComponentPrefix ValueType;
  static constexpr const char* tagname = "prefix";
  static constexpr const char* keyname = "norm";
};
using NormDependentPrefixMap =
    SerializableKeyValueMap<NormDependentPrefixMapPolicy>;

/*******************************************************************************
 *  Class Component
 ******************************************************************************/

/**
 * @brief The Component class represents a "generic" device in the library
 *
 * Following information is considered as the "interface" of a component and
 * must therefore never be changed:
 *  - UUID
 *  - Property "is schematic only"
 *  - All signal UUIDs (and their meaning)
 *  - Symbol variants (adding new variants is allowed, but removing not)
 *    - UUID
 *    - Symbol items (neither adding nor removing items is allowed)
 *      - UUID
 *      - Symbol UUID
 *      - Pin-signal-mapping
 */
class Component final : public LibraryElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  Component() = delete;
  Component(const Component& other) = delete;
  Component(const Uuid& uuid, const Version& version, const QString& author,
            const ElementName& name_en_US, const QString& description_en_US,
            const QString& keywords_en_US);
  ~Component() noexcept;

  // General
  bool isSchematicOnly() const noexcept { return mSchematicOnly; }
  void setIsSchematicOnly(bool schematicOnly) noexcept {
    mSchematicOnly = schematicOnly;
  }

  // Attribute Methods
  AttributeList& getAttributes() noexcept { return mAttributes; }
  const AttributeList& getAttributes() const noexcept { return mAttributes; }

  // Default Value Methods
  const QString& getDefaultValue() const noexcept { return mDefaultValue; }
  void setDefaultValue(const QString& value) noexcept { mDefaultValue = value; }

  // Prefix Methods
  const NormDependentPrefixMap& getPrefixes() const noexcept {
    return mPrefixes;
  }
  void setPrefixes(const NormDependentPrefixMap& prefixes) noexcept {
    mPrefixes = prefixes;
  }

  // Signal Methods
  ComponentSignalList& getSignals() noexcept { return mSignals; }
  const ComponentSignalList& getSignals() const noexcept { return mSignals; }

  // Symbol Variant Methods
  ComponentSymbolVariantList& getSymbolVariants() noexcept {
    return mSymbolVariants;
  }
  const ComponentSymbolVariantList& getSymbolVariants() const noexcept {
    return mSymbolVariants;
  }

  // Convenience Methods
  std::shared_ptr<ComponentSignal> getSignalOfPin(const Uuid& symbVar,
                                                  const Uuid& item,
                                                  const Uuid& pin);
  std::shared_ptr<const ComponentSignal> getSignalOfPin(const Uuid& symbVar,
                                                        const Uuid& item,
                                                        const Uuid& pin) const;
  int getSymbolVariantIndexByNorm(const QStringList& normOrder) const noexcept;
  std::shared_ptr<ComponentSymbolVariantItem> getSymbVarItem(
      const Uuid& symbVar, const Uuid& item);
  std::shared_ptr<const ComponentSymbolVariantItem> getSymbVarItem(
      const Uuid& symbVar, const Uuid& item) const;

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;

  // Operator Overloadings
  Component& operator=(const Component& rhs) = delete;

  // Static Methods
  static std::unique_ptr<Component> open(
      std::unique_ptr<TransactionalDirectory> directory);
  static QString getShortElementName() noexcept {
    return QStringLiteral("cmp");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("component");
  }

protected:  // Methods
  virtual void serialize(SExpression& root) const override;

private:  // Methods
  Component(std::unique_ptr<TransactionalDirectory> directory,
            const SExpression& root);
  static QString cleanNorm(QString norm) noexcept;

private:  // Data
  bool mSchematicOnly;  ///< if true, this component is schematic-only (no
                        ///< package)
  QString mDefaultValue;
  NormDependentPrefixMap mPrefixes;
  AttributeList mAttributes;  ///< all attributes in a specific order
  ComponentSignalList mSignals;
  ComponentSymbolVariantList mSymbolVariants;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
