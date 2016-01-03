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

#ifndef PROJECT_COMPONENTATTRIBUTEINSTANCE_H
#define PROJECT_COMPONENTATTRIBUTEINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class AttributeType;
class AttributeUnit;

/*****************************************************************************************
 *  Class ComponentAttributeInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The ComponentAttributeInstance class
 */
class ComponentAttributeInstance final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentAttributeInstance)

    public:

        // Constructors / Destructor
        explicit ComponentAttributeInstance(const XmlDomElement& domElement) throw (Exception);
        explicit ComponentAttributeInstance(const QString& key, const AttributeType& type,
                                            const QString& value, const AttributeUnit* unit) throw (Exception);
        ~ComponentAttributeInstance() noexcept;

        // Getters
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


    private:

        // make some methods inaccessible...
        ComponentAttributeInstance();
        ComponentAttributeInstance(const ComponentAttributeInstance& other);
        ComponentAttributeInstance& operator=(const ComponentAttributeInstance& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        QString mKey;
        const AttributeType* mType;
        QString mValue;
        const AttributeUnit* mUnit;
};

} // namespace project

#endif // PROJECT_COMPONENTATTRIBUTEINSTANCE_H
