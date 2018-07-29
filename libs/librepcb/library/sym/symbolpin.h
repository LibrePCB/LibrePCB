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

#ifndef LIBREPCB_LIBRARY_SYMBOLPIN_H
#define LIBREPCB_LIBRARY_SYMBOLPIN_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/circuitidentifier.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

class SymbolPinGraphicsItem;

/*****************************************************************************************
 *  Class SymbolPin
 ****************************************************************************************/

/**
 * @brief The SymbolPin class represents one pin of a symbol
 *
 * Following information is considered as the "interface" of a pin and must therefore
 * never be changed:
 *  - UUID
 */
class SymbolPin final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(SymbolPin)

    public:

        // Constructors / Destructor
        SymbolPin() = delete;
        SymbolPin(const SymbolPin& other) noexcept;
        SymbolPin(const Uuid& uuid, const CircuitIdentifier& name, const Point& position,
                  const UnsignedLength& length, const Angle& rotation) noexcept;
        explicit SymbolPin(const SExpression& node);
        ~SymbolPin() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const CircuitIdentifier& getName() const noexcept {return mName;}
        const Point& getPosition() const noexcept {return mPosition;}
        const UnsignedLength& getLength() const noexcept {return mLength;}
        const Angle& getRotation() const noexcept {return mRotation;}

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setLength(const UnsignedLength& length) noexcept;
        void setRotation(const Angle& rotation) noexcept;
        void setName(const CircuitIdentifier& name) noexcept;

        // General Methods
        void registerGraphicsItem(SymbolPinGraphicsItem& item) noexcept;
        void unregisterGraphicsItem(SymbolPinGraphicsItem& item) noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const SymbolPin& rhs) const noexcept;
        bool operator!=(const SymbolPin& rhs) const noexcept {return !(*this == rhs);}
        SymbolPin& operator=(const SymbolPin& rhs) noexcept;


    private: // Data
        Uuid mUuid;
        CircuitIdentifier mName;
        Point mPosition;
        UnsignedLength mLength;
        Angle mRotation;

        SymbolPinGraphicsItem* mRegisteredGraphicsItem;
};

/*****************************************************************************************
 *  Class SymbolPinList
 ****************************************************************************************/

struct SymbolPinListNameProvider {static constexpr const char* tagname = "pin";};
using SymbolPinList = SerializableObjectList<SymbolPin, SymbolPinListNameProvider>;
using CmdSymbolPinInsert = CmdListElementInsert<SymbolPin, SymbolPinListNameProvider>;
using CmdSymbolPinRemove = CmdListElementRemove<SymbolPin, SymbolPinListNameProvider>;
using CmdSymbolPinsSwap = CmdListElementsSwap<SymbolPin, SymbolPinListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_SYMBOLPIN_H
