/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef PROJECT_SI_SYMBOL_H
#define PROJECT_SI_SYMBOL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/if_attributeprovider.h>
#include "../graphicsitems/sgi_symbol.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Schematic;
class GenCompInstance;
class SI_SymbolPin;
}

namespace library {
class Symbol;
class ComponentSymbolVariantItem;
}

/*****************************************************************************************
 *  Class SI_Symbol
 ****************************************************************************************/

namespace project {

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
        explicit SI_Symbol(Schematic& schematic, const XmlDomElement& domElement) throw (Exception);
        explicit SI_Symbol(Schematic& schematic, GenCompInstance& genCompInstance,
                           const QUuid& symbolItem, const Point& position = Point(),
                           const Angle& rotation = Angle()) throw (Exception);
        ~SI_Symbol() noexcept;

        // Getters
        Project& getProject() const noexcept;
        Schematic& getSchematic() const noexcept {return mSchematic;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Angle& getRotation() const noexcept {return mRotation;}
        QString getName() const noexcept;
        SI_SymbolPin* getPin(const QUuid& pinUuid) const noexcept {return mPins.value(pinUuid);}
        const QHash<QUuid, SI_SymbolPin*>& getPins() const noexcept {return mPins;}
        GenCompInstance& getGenCompInstance() const noexcept {return *mGenCompInstance;}
        const library::Symbol& getLibSymbol() const noexcept {return *mSymbol;}
        const library::ComponentSymbolVariantItem& getGenCompSymbVarItem() const noexcept {return *mSymbVarItem;}

        // Setters
        void setPosition(const Point& newPos) throw (Exception);
        void setRotation(const Angle& newRotation) throw (Exception);

        // General Methods
        void addToSchematic(GraphicsScene& scene) throw (Exception);
        void removeFromSchematic(GraphicsScene& scene) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // Helper Methods
        Point mapToScene(const Point& relativePos) const noexcept;
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;

        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::Symbol;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


    private slots:

        void schematicOrGenCompAttributesChanged();


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();


    private:

        // make some methods inaccessible...
        SI_Symbol();
        SI_Symbol(const SI_Symbol& other);
        SI_Symbol& operator=(const SI_Symbol& rhs);

        // Private Methods
        void init(const QUuid& symbVarItemUuid) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        Schematic& mSchematic;
        GenCompInstance* mGenCompInstance;
        const library::ComponentSymbolVariantItem* mSymbVarItem;
        const library::Symbol* mSymbol;
        QHash<QUuid, SI_SymbolPin*> mPins; ///< key: symbol pin UUID
        SGI_Symbol* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        Point mPosition;
        Angle mRotation;
};

} // namespace project

#endif // PROJECT_SI_SYMBOL_H
