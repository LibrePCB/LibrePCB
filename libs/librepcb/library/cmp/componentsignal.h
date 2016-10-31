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

#ifndef LIBREPCB_LIBRARY_COMPONENTSIGNAL_H
#define LIBREPCB_LIBRARY_COMPONENTSIGNAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class ComponentSignal
 ****************************************************************************************/

/**
 * @brief The ComponentSignal class
 */
class ComponentSignal final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(ComponentSignal)

    public:

        // Types
        enum class SignalRole_t {
            POWER,     ///< Power Pins (GND, VCC, VSS,... of Devices)
            INPUT,     ///< Input Pins
            OUTPUT,    ///< Output Pins
            INOUT,     ///< Input/Output Pins
            OPENDRAIN, ///< Open Collector / Open Drain Pins
            PASSIVE,   ///< Passive Pins (R, C, L)
        };


        // Constructors / Destructor
        explicit ComponentSignal(const Uuid& uuid, const QString& name) noexcept;
        explicit ComponentSignal(const XmlDomElement& domElement) throw (Exception);
        ~ComponentSignal() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        QString getName() const noexcept {return mName;}
        SignalRole_t getRole() const noexcept {return mRole;}
        const QString& getForcedNetName() const noexcept {return mForcedNetName;}
        bool isRequired() const noexcept {return mIsRequired;}
        bool isNegated() const noexcept {return mIsNegated;}
        bool isClock() const noexcept {return mIsClock;}
        bool isNetSignalNameForced() const noexcept {return !mForcedNetName.isEmpty();}

        // Setters
        void setName(const QString& name) noexcept {mName = name;}
        void setRole(SignalRole_t role) noexcept {mRole = role;}
        void setForcedNetName(const QString& name) noexcept {mForcedNetName = name;}
        void setIsRequired(bool required) noexcept {mIsRequired = required;}
        void setIsNegated(bool negated) noexcept {mIsNegated = negated;}
        void setIsClock(bool clock) noexcept {mIsClock = clock;}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        ComponentSignal() = delete;
        ComponentSignal(const ComponentSignal& other) = delete;
        ComponentSignal& operator=(const ComponentSignal& rhs) = delete;

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Private Static Methods
        static SignalRole_t stringToSignalRole(const QString& role) throw (Exception);
        static QString signalRoleToString(SignalRole_t role) noexcept;


        // Signal Attributes
        Uuid mUuid;
        QString mName;
        SignalRole_t mRole;
        QString mForcedNetName;
        bool mIsRequired;
        bool mIsNegated;
        bool mIsClock;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_COMPONENTSIGNAL_H
