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

#ifndef LIBREPCB_IF_XMLSERIALIZABLEOBJECT_H
#define LIBREPCB_IF_XMLSERIALIZABLEOBJECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "../exceptions.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class XmlDomElement;

/*****************************************************************************************
 *  Class IF_XmlSerializableObject
 ****************************************************************************************/

/**
 * @brief The IF_XmlSerializableObject class provides an interface for classes which
 *          are serializable/deserializable from/to XML DOM elements
 *
 * @author ubruhin
 * @date 2015-02-06
 */
class IF_XmlSerializableObject
{
    public:

        /**
         * @brief Default Constructor
         */
        explicit IF_XmlSerializableObject() noexcept {}

        /**
         * @brief Destructor
         */
        virtual ~IF_XmlSerializableObject() noexcept {}


        /**
         * @brief Serialize the object to a XML DOM element
         *
         * This is a pure virtual method which must be implemented in all subclasses of
         * the interface #IF_XmlSerializableObject. The generated XML DOM element has
         * always the format of the application's major version (it's not possible to
         * generate XML files of older versions).
         *
         * @return The created XML DOM element (the caller takes the ownership!)
         *
         * @throw Exception     This method throws an exception if an error occurs.
         *
         * @todo maybe better return a QSharedPointer instead of a raw pointer?
         */
        virtual XmlDomElement* serializeToXmlDomElement() const throw (Exception) = 0;


    protected:

        /**
         * @brief Check the validity of all attributes of the object
         *
         * This method is useful to check the validity of all object attributes after
         * deserialization and before serialization to avoid loading or creating invalid
         * XML files.
         *
         * @retval true     If all attributes are valid
         * @retval false    If at least one attribute is invalid
         */
        virtual bool checkAttributesValidity() const noexcept = 0;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_IF_XMLSERIALIZABLEOBJECT_H
