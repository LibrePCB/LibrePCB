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

#ifndef LIBREPCB_PROJECT_SI_SYMBOL_H
#define LIBREPCB_PROJECT_SI_SYMBOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphicsitems/sgi_symbol.h"
#include "si_base.h"

#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/fileio/serializableobject.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace library {
class Symbol;
class ComponentSymbolVariantItem;
}  // namespace library

namespace project {

class Schematic;
class ComponentInstance;
class SI_SymbolPin;

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
  SI_Symbol()                       = delete;
  SI_Symbol(const SI_Symbol& other) = delete;
  explicit SI_Symbol(Schematic& schematic, const SExpression& node);
  explicit SI_Symbol(Schematic& schematic, ComponentInstance& cmpInstance,
                     const Uuid& symbolItem, const Point& position = Point(),
                     const Angle& rotation = Angle(), bool mirrored = false);
  ~SI_Symbol() noexcept;

  // Getters
  const Uuid&   getUuid() const noexcept { return mUuid; }
  const Angle&  getRotation() const noexcept { return mRotation; }
  bool          getMirrored() const noexcept { return mMirrored; }
  QString       getName() const noexcept;
  SI_SymbolPin* getPin(const Uuid& pinUuid) const noexcept {
    return mPins.value(pinUuid);
  }
  const QHash<Uuid, SI_SymbolPin*>& getPins() const noexcept { return mPins; }
  ComponentInstance&                getComponentInstance() const noexcept {
    return *mComponentInstance;
  }
  const library::Symbol& getLibSymbol() const noexcept { return *mSymbol; }
  const library::ComponentSymbolVariantItem& getCompSymbVarItem() const
      noexcept {
    return *mSymbVarItem;
  }

  // Setters
  void setPosition(const Point& newPos) noexcept;
  void setRotation(const Angle& newRotation) noexcept;
  void setMirrored(bool newMirrored) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Helper Methods
  Point mapToScene(const Point& relativePos) const noexcept;

  // Inherited from AttributeProvider
  /// @copydoc librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Inherited from SI_Base
  Type_t getType() const noexcept override { return SI_Base::Type_t::Symbol; }
  const Point& getPosition() const noexcept override { return mPosition; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void         setSelected(bool selected) noexcept override;

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
  ComponentInstance*                         mComponentInstance;
  const library::ComponentSymbolVariantItem* mSymbVarItem;
  const library::Symbol*                     mSymbol;
  QHash<Uuid, SI_SymbolPin*>                 mPins;  ///< key: symbol pin UUID
  QScopedPointer<SGI_Symbol>                 mGraphicsItem;

  // Attributes
  Uuid  mUuid;
  Point mPosition;
  Angle mRotation;
  bool  mMirrored;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SI_SYMBOL_H
