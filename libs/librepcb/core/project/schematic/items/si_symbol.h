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

#ifndef LIBREPCB_CORE_SI_SYMBOL_H
#define LIBREPCB_CORE_SI_SYMBOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../attribute/attributeprovider.h"
#include "../../../serialization/serializableobject.h"
#include "../graphicsitems/sgi_symbol.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class ComponentSymbolVariantItem;
class SI_SymbolPin;
class Schematic;
class Symbol;

/*******************************************************************************
 *  Class SI_Symbol
 ******************************************************************************/

/**
 * @brief The SI_Symbol class
 */
class SI_Symbol final : public SI_Base,
                        public SerializableObject,
                        public AttributeProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_Symbol() = delete;
  SI_Symbol(const SI_Symbol& other) = delete;
  SI_Symbol(Schematic& schematic, const SExpression& node,
            const Version& fileFormat);
  explicit SI_Symbol(Schematic& schematic, ComponentInstance& cmpInstance,
                     const Uuid& symbolItem, const Point& position = Point(),
                     const Angle& rotation = Angle(), bool mirrored = false);
  ~SI_Symbol() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  bool getMirrored() const noexcept { return mMirrored; }
  QString getName() const noexcept;
  SI_SymbolPin* getPin(const Uuid& pinUuid) const noexcept {
    return mPins.value(pinUuid);
  }
  const QHash<Uuid, SI_SymbolPin*>& getPins() const noexcept { return mPins; }
  ComponentInstance& getComponentInstance() const noexcept {
    return *mComponentInstance;
  }
  const Symbol& getLibSymbol() const noexcept { return *mSymbol; }
  const ComponentSymbolVariantItem& getCompSymbVarItem() const noexcept {
    return *mSymbVarItem;
  }
  QRectF getBoundingRect() const noexcept;

  // Setters
  void setPosition(const Point& newPos) noexcept;
  void setRotation(const Angle& newRotation) noexcept;
  void setMirrored(bool newMirrored) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from AttributeProvider
  /// @copydoc ::librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc ::librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Inherited from SI_Base
  Type_t getType() const noexcept override { return SI_Base::Type_t::Symbol; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  SI_Symbol& operator=(const SI_Symbol& rhs) = delete;

private slots:

  void schematicOrComponentAttributesChanged();

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

private:
  void init(const Uuid& symbVarItemUuid);
  void updateGraphicsItemTransform() noexcept;
  bool checkAttributesValidity() const noexcept;

  // General
  ComponentInstance* mComponentInstance;
  const ComponentSymbolVariantItem* mSymbVarItem;
  const Symbol* mSymbol;
  QHash<Uuid, SI_SymbolPin*> mPins;  ///< key: symbol pin UUID
  QScopedPointer<SGI_Symbol> mGraphicsItem;

  // Attributes
  Uuid mUuid;
  Point mPosition;
  Angle mRotation;
  bool mMirrored;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
