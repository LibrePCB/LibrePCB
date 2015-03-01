/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_GENCOMPATTRIBUTEINSTANCE_H
#define PROJECT_GENCOMPATTRIBUTEINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/file_io/if_xmlserializableobject.h"
#include "../../common/exceptions.h"
#include "../../library/attribute.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace project {
class Circuit;
class GenCompInstance;
}

/*****************************************************************************************
 *  Class GenCompAttributeInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenCompAttributeInstance class
 */
class GenCompAttributeInstance final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(GenCompAttributeInstance)

    public:

        // Constructors / Destructor
        explicit GenCompAttributeInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                          const XmlDomElement& domElement) throw (Exception);
        explicit GenCompAttributeInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                          const QString& key, library::Attribute::Type_t type,
                                          const QString& value) throw (Exception);
        ~GenCompAttributeInstance() noexcept;

        // Getters
        const QString& getKey() const noexcept {return mKey;}
        library::Attribute::Type_t getType() const noexcept {return mType;}
        QString getValueToDisplay() const noexcept;

        // General Methods
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


    private:

        // make some methods inaccessible...
        GenCompAttributeInstance();
        GenCompAttributeInstance(const GenCompAttributeInstance& other);
        GenCompAttributeInstance& operator=(const GenCompAttributeInstance& rhs);

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // General
        Circuit& mCircuit;
        GenCompInstance& mGenCompInstance;

        // Attributes
        QString mKey;
        library::Attribute::Type_t mType;
        QString mValue;
};

} // namespace project

#endif // PROJECT_GENCOMPATTRIBUTEINSTANCE_H
