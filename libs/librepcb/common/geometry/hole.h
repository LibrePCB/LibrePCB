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

#ifndef LIBREPCB_HOLE_H
#define LIBREPCB_HOLE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobjectlist.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Interface IF_HoleObserver
 ****************************************************************************************/

/**
 * @brief The IF_HoleObserver class
 *
 * @author ubruhin
 * @date 2017-05-20
 */
class IF_HoleObserver
{
    public:
        virtual void holePositionChanged(const Point& newPos) noexcept = 0;
        virtual void holeDiameterChanged(const Length& newDiameter) noexcept = 0;

    protected:
        IF_HoleObserver() noexcept {}
        explicit IF_HoleObserver(const IF_HoleObserver& other) = delete;
        virtual ~IF_HoleObserver() noexcept {}
        IF_HoleObserver& operator=(const IF_HoleObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class Hole
 ****************************************************************************************/

/**
 * @brief The Hole class
 */
class Hole final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Hole)

    public:

        // Constructors / Destructor
        Hole() = delete;
        Hole(const Hole& other) noexcept;
        Hole(const Point& position, const Length& diameter) noexcept;
        explicit Hole(const SExpression& node);
        ~Hole() noexcept;

        // Getters
        const Point& getPosition() const noexcept {return mPosition;}
        const Length& getDiameter() const noexcept {return mDiameter;}

        // Setters
        void setPosition(const Point& position) noexcept;
        void setDiameter(const Length& diameter) noexcept;

        // General Methods
        void registerObserver(IF_HoleObserver& object) const noexcept;
        void unregisterObserver(IF_HoleObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const Hole& rhs) const noexcept;
        bool operator!=(const Hole& rhs) const noexcept {return !(*this == rhs);}
        Hole& operator=(const Hole& rhs) noexcept;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Point mPosition;
        Length mDiameter;

        // Misc
        mutable QSet<IF_HoleObserver*> mObservers; ///< A list of all observer objects
};

/*****************************************************************************************
 *  Class HoleList
 ****************************************************************************************/

struct HoleListNameProvider {static constexpr const char* tagname = "hole";};
using HoleList = SerializableObjectList<Hole, HoleListNameProvider>;
using CmdHoleInsert = CmdListElementInsert<Hole, HoleListNameProvider>;
using CmdHoleRemove = CmdListElementRemove<Hole, HoleListNameProvider>;
using CmdHolesSwap = CmdListElementsSwap<Hole, HoleListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_HOLE_H
