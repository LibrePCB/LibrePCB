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

#ifndef LIBREPCB_ATTRIBUTE_H
#define LIBREPCB_ATTRIBUTE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobjectlist.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class AttributeType;
class AttributeUnit;

/*****************************************************************************************
 *  Class Attribute
 ****************************************************************************************/

/**
 * @brief The Attribute class
 */
class Attribute final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Attribute)

    public:

        // Constructors / Destructor
        Attribute() = delete;
        Attribute(const Attribute& other) noexcept;
        explicit Attribute(const XmlDomElement& domElement) throw (Exception);
        Attribute(const QString& key, const AttributeType& type, const QString& value,
                  const AttributeUnit* unit) throw (Exception);
        ~Attribute() noexcept;

        // Getters
        const QString& getKey() const noexcept {return mKey;}
        const QString& getName() const noexcept {return mKey;} // required for SerializableObjectList
        const AttributeType& getType() const noexcept {return *mType;}
        const AttributeUnit* getUnit() const noexcept {return mUnit;}
        const QString& getValue() const noexcept {return mValue;}
        QString getValueTr(bool showUnit) const noexcept;

        // Setters
        void setKey(const QString& key) throw (Exception);
        void setTypeValueUnit(const AttributeType& type, const QString& value,
                              const AttributeUnit* unit) throw (Exception);

        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(XmlDomElement& root) const throw (Exception) override;

        // Operator Overloadings
        bool operator==(const Attribute& rhs) const noexcept;
        bool operator!=(const Attribute& rhs) const noexcept {return !(*this == rhs);}
        Attribute& operator=(const Attribute& rhs) = delete;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        QString mKey;
        const AttributeType* mType;
        QString mValue;
        const AttributeUnit* mUnit;
};

/*****************************************************************************************
 *  Class AttributeList
 ****************************************************************************************/

struct AttributeListNameProvider {static constexpr const char* tagname = "attribute";};
using AttributeList = SerializableObjectList<Attribute, AttributeListNameProvider>;
using CmdAttributeInsert = CmdListElementInsert<Attribute, AttributeListNameProvider>;
using CmdAttributeRemove = CmdListElementRemove<Attribute, AttributeListNameProvider>;
using CmdAttributesSwap = CmdListElementsSwap<Attribute, AttributeListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ATTRIBUTE_H
