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

#ifndef LIBREPCB_SIGNALROLE_H
#define LIBREPCB_SIGNALROLE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "exceptions.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class SignalRole
 ****************************************************************************************/

/**
 * @brief The SignalRole class provides all supported electrical signal roles
 *
 * @author ubruhin
 * @date 2016-10-20
 */
class SignalRole final
{
        Q_DECLARE_TR_FUNCTIONS(SignalRole)

    public:

        // Constructors / Destructor
        SignalRole() noexcept;
        SignalRole(const SignalRole& other) noexcept;
        ~SignalRole() noexcept;


        // Getters

        /**
         * @brief Serialize this object into a string
         *
         * @return This object as a string
         */
        QString serializeToString() const noexcept {return mRole;}

        /**
         * @brief Get the name of the SignalRole (human readable and translated)
         *
         * @return The name of the SignalRole
         */
        const QString& getNameTr() const noexcept {return mName;}


        // Operator Overloadings
        SignalRole& operator=(const SignalRole& rhs) noexcept;
        bool operator==(const SignalRole& rhs) const noexcept {return mRole == rhs.mRole;}
        bool operator!=(const SignalRole& rhs) const noexcept {return mRole != rhs.mRole;}


        // Static Methods

        /**
         * @brief Deserialize object from a string
         *
         * @param str           Input string
         *
         * @return The created element
         *
         * @throws Exception if the string was invalid
         */
        static const SignalRole& deserializeFromString(const QString& str);

        /**
         * @brief Get a list of all available signal roles
         *
         * @return A list of all roles
         */
        static const QList<SignalRole>& getAllRoles() noexcept;

        /// @brief Passive Pins (R, C, L)
        static const SignalRole& passive() noexcept {
            static SignalRole role("passive", tr("Passive"));
            return role;
        }

        /// @brief Power Pins (GND, VCC, VSS,... of Devices)
        static const SignalRole& power() noexcept {
            static SignalRole role("power", tr("Power"));
            return role;
        }

        /// @brief Input Pins
        static const SignalRole& input() noexcept {
            static SignalRole role("input", tr("Input"));
            return role;
        }

        /// @brief Output Pins
        static const SignalRole& output() noexcept {
            static SignalRole role("output", tr("Output"));
            return role;
        }

        /// @brief Input/Output Pins
        static const SignalRole& inout() noexcept {
            static SignalRole role("inout", tr("I/O"));
            return role;
        }

        /// @brief Open Collector / Open Drain Pins
        static const SignalRole& opendrain() noexcept {
            static SignalRole role("opendrain", tr("Open Drain"));
            return role;
        }


    private: // Methods
        SignalRole(const QString& role, const QString& name) noexcept;


    private: // Data
        QString mRole;  ///< used for serialization (DO NOT MODIFY VALUES!)
        QString mName;  ///< human readable (translated)
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_SIGNALROLE_H
