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

#ifndef LIBREPCB_PROJECT_COMPONENTATTRIBUTEINSTANCE_H
#define LIBREPCB_PROJECT_COMPONENTATTRIBUTEINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class AttributeType;
class AttributeUnit;

namespace project {

class ComponentInstance;

/*****************************************************************************************
 *  Class ComponentAttributeInstance
 ****************************************************************************************/

/**
 * @brief The ComponentAttributeInstance class
 */
class ComponentAttributeInstance final : public QObject, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        ComponentAttributeInstance() = delete;
        ComponentAttributeInstance(const ComponentAttributeInstance& other) = delete;
        ComponentAttributeInstance(ComponentInstance& cmp, const XmlDomElement& domElement) throw (Exception);
        ComponentAttributeInstance(ComponentInstance& cmp, const QString& key,
                                   const AttributeType& type, const QString& value,
                                   const AttributeUnit* unit) throw (Exception);
        ~ComponentAttributeInstance() noexcept;

        // Getters
        ComponentInstance& getComponentInstance() const noexcept {return mComponentInstance;}
        const QString& getKey() const noexcept {return mKey;}
        const AttributeType& getType() const noexcept {return *mType;}
        const AttributeUnit* getUnit() const noexcept {return mUnit;}
        const QString& getValue() const noexcept {return mValue;}
        QString getValueTr(bool showUnit) const noexcept;

        // Setters
        void setTypeValueUnit(const AttributeType& type, const QString& value,
                              const AttributeUnit* unit) throw (Exception);

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        ComponentAttributeInstance& operator=(const ComponentAttributeInstance& rhs) = delete;


    private:

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        ComponentInstance& mComponentInstance;
        QString mKey;
        const AttributeType* mType;
        QString mValue;
        const AttributeUnit* mUnit;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_COMPONENTATTRIBUTEINSTANCE_H
