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

#ifndef LIBREPCB_PROJECT_SI_SYMBOL_H
#define LIBREPCB_PROJECT_SI_SYMBOL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "si_base.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/if_attributeprovider.h>
#include "../graphicsitems/sgi_symbol.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Symbol;
class ComponentSymbolVariantItem;
}

namespace project {

class Schematic;
class ComponentInstance;
class SI_SymbolPin;

/*****************************************************************************************
 *  Class SI_Symbol
 ****************************************************************************************/

/**
 * @brief The SI_Symbol class
 *
 * @author ubruhin
 * @date 2014-08-23
 */
class SI_Symbol final : public SI_Base, public IF_XmlSerializableObject,
                        public IF_AttributeProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        SI_Symbol() = delete;
        SI_Symbol(const SI_Symbol& other) = delete;
        explicit SI_Symbol(Schematic& schematic, const XmlDomElement& domElement) throw (Exception);
        explicit SI_Symbol(Schematic& schematic, ComponentInstance& cmpInstance,
                           const Uuid& symbolItem, const Point& position = Point(),
                           const Angle& rotation = Angle()) throw (Exception);
        ~SI_Symbol() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const Angle& getRotation() const noexcept {return mRotation;}
        QString getName() const noexcept;
        SI_SymbolPin* getPin(const Uuid& pinUuid) const noexcept {return mPins.value(pinUuid);}
        const QHash<Uuid, SI_SymbolPin*>& getPins() const noexcept {return mPins;}
        ComponentInstance& getComponentInstance() const noexcept {return *mComponentInstance;}
        const library::Symbol& getLibSymbol() const noexcept {return *mSymbol;}
        const library::ComponentSymbolVariantItem& getCompSymbVarItem() const noexcept {return *mSymbVarItem;}

        // Setters
        void setPosition(const Point& newPos) noexcept;
        void setRotation(const Angle& newRotation) noexcept;

        // General Methods
        void addToSchematic(GraphicsScene& scene) throw (Exception) override;
        void removeFromSchematic(GraphicsScene& scene) throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // Helper Methods
        Point mapToScene(const Point& relativePos) const noexcept;
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept override;

        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::Symbol;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        SI_Symbol& operator=(const SI_Symbol& rhs) = delete;


    private slots:

        void schematicOrComponentAttributesChanged();


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged() override;


    private:

        void init(const Uuid& symbVarItemUuid) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        ComponentInstance* mComponentInstance;
        const library::ComponentSymbolVariantItem* mSymbVarItem;
        const library::Symbol* mSymbol;
        QHash<Uuid, SI_SymbolPin*> mPins; ///< key: symbol pin UUID
        QScopedPointer<SGI_Symbol> mGraphicsItem;

        // Attributes
        Uuid mUuid;
        Point mPosition;
        Angle mRotation;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SI_SYMBOL_H
