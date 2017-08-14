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

#ifndef LIBREPCB_XMLSERIALIZABLEOBJECT_H
#define LIBREPCB_XMLSERIALIZABLEOBJECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "domelement.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class SerializableObject
 ****************************************************************************************/

/**
 * @brief The SerializableObject class is the base class for all classes which need to be
 *        serializable/deserializable from/to DOM elements
 *
 * @author ubruhin
 * @date 2015-02-06
 */
class SerializableObject
{
    public:
        SerializableObject() noexcept {}
        virtual ~SerializableObject() noexcept {}


        /**
         * @brief Serialize the object to a new DOM element
         *
         * This method creates a new DOM element, serializes the whole object into it and
         * then returns the whole DOM element. See #serialize() for details.
         *
         * @param name          The root name of the returned DOM element
         *
         * @return The created XML DOM element (the caller takes the ownership!)
         *
         * @throw Exception     This method throws an exception if an error occurs.
         *
         * @see #serialize()
         */
        DomElement* serializeToDomElement(const QString& name) const {
            QScopedPointer<DomElement> root(new DomElement(name));
            serialize(*root);
            return root.take();
        }

        /**
         * @brief Serialize the object into an existing DOM element
         *
         * This method inserts/appends all attributes and childs of the object to an
         * existing DOM element. The content which already exists in the given DOM element
         * will not be removed.
         *
         * @note    The generated DOM element has always the format of the application's
         *          major version (it's not possible to generate DOMs of older versions).
         *
         * @param root          The target DOM root node
         *
         * @throw Exception     This method throws an exception if an error occurs.
         */
        virtual void serialize(DomElement& root) const = 0;

        template <typename T>
        static void serializeObjectContainer(DomElement& root, const T& container,
            const QString& itemName)
        {
            for (const auto& object : container) {
                root.appendChild(object.serializeToDomElement(itemName)); // can throw
            }
        }

        template <typename T>
        static void serializePointerContainer(DomElement& root, const T& container,
            const QString& itemName)
        {
            for (const auto& pointer : container) {
                root.appendChild(pointer->serializeToDomElement(itemName)); // can throw
            }
        }
};

// Make sure that the SerializableObject class does not contain any data (except the vptr).
// Otherwise it could introduce issues when using multiple inheritance.
static_assert(sizeof(SerializableObject) == sizeof(void*),
              "SerializableObject must not contain any data!");

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_XMLSERIALIZABLEOBJECT_H
