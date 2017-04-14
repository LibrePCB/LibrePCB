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

#ifndef LIBREPCB_ATTRIBUTELIST_H
#define LIBREPCB_ATTRIBUTELIST_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobject.h"
#include "attribute.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class AttributeList
 ****************************************************************************************/

/**
 * @brief The AttributeList class
 */
class AttributeList final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(AttributeList)

    public:

        // Constructors / Destructor
        AttributeList() noexcept;
        AttributeList(const AttributeList& other) noexcept;
        explicit AttributeList(const XmlDomElement& domElement) throw (Exception);
        ~AttributeList() noexcept;

        // Getters
        int count() const noexcept {return mAttributes.count();}
        bool contains(int index) const noexcept {return index >= 0 && index < count();}
        bool contains(const QString& key) const noexcept {return value(key) != nullptr;}
        int indexOf(const QString& key) const noexcept;
        Attribute* value(int index) noexcept {return mAttributes.value(index).data();}
        const Attribute* value(int index) const noexcept {return mAttributes.value(index).data();}
        Attribute* value(const QString& key) noexcept {return mAttributes.value(indexOf(key)).data();}
        const Attribute* value(const QString& key) const noexcept {return mAttributes.value(indexOf(key)).data();}

        // General Methods
        void swap(int i, int j) noexcept {mAttributes.swap(i, j);}
        void append(const Attribute& attr) noexcept
            {mAttributes.append(QSharedPointer<Attribute>::create(attr));}
        void insert(int index, const Attribute& attr) noexcept
            {mAttributes.insert(index, QSharedPointer<Attribute>::create(attr));}
        void remove(int index) noexcept
            {if (contains(index)) mAttributes.removeAt(index);}
        void remove(const QString& key) noexcept
            {int i = indexOf(key); if (contains(i)) mAttributes.removeAt(i);}
        void clear() noexcept {mAttributes.clear();}

        /// @copydoc SerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        bool operator==(const AttributeList& rhs) const noexcept;
        bool operator!=(const AttributeList& rhs) const noexcept {return !(*this == rhs);}
        AttributeList& operator=(const AttributeList& rhs) noexcept;


    private: // Methods

        /// @copydoc SerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


    private: // Data
        QList<QSharedPointer<Attribute>> mAttributes;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ATTRIBUTELIST_H
