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

#ifndef LIBRARY_GENCOMPSIGNAL_H
#define LIBRARY_GENCOMPSIGNAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class GenCompSignal
 ****************************************************************************************/

namespace library {

/**
 * @brief The GenCompSignal class
 */
class GenCompSignal final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(GenCompSignal)

    public:

        // Types
        enum class SignalRole_t {
            Power,     ///< Power Pins (GND, VCC, VSS,... of Devices)
            Input,     ///< Input Pins
            Output,    ///< Output Pins
            InOut,     ///< Input/Output Pins
            OpenDrain, ///< Open Collector / Open Drain Pins
            Passive,   ///< Passive Pins (R, C, L)
        };


        // Constructors / Destructor
        explicit GenCompSignal(const QUuid& uuid = QUuid::createUuid(),
                               const QString& name_en_US = QString(),
                               const QString& description_en_US = QString()) noexcept;
        explicit GenCompSignal(const XmlDomElement& domElement) throw (Exception);
        ~GenCompSignal() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        SignalRole_t getRole() const noexcept {return mRole;}
        const QString& getForcedNetName() const noexcept {return mForcedNetName;}
        bool isRequired() const noexcept {return mIsRequired;}
        bool isNegated() const noexcept {return mIsNegated;}
        bool isClock() const noexcept {return mIsClock;}
        bool isNetSignalNameForced() const noexcept {return !mForcedNetName.isEmpty();}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}

        // Setters
        void setRole(SignalRole_t role) noexcept {mRole = role;}
        void setForcedNetName(const QString& name) noexcept {mForcedNetName = name;}
        void setIsRequired(bool required) noexcept {mIsRequired = required;}
        void setIsNegated(bool negated) noexcept {mIsNegated = negated;}
        void setIsClock(bool clock) noexcept {mIsClock = clock;}
        void setName(const QString& locale, const QString& name) noexcept {mNames[locale] = name;}
        void setDescription(const QString& locale, const QString& desc) noexcept {mDescriptions[locale] = desc;}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement(int version) const throw (Exception) override;


    private:

        // make some methods inaccessible...
        GenCompSignal(const GenCompSignal& other);
        GenCompSignal& operator=(const GenCompSignal& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Private Static Methods
        static SignalRole_t stringToSignalRole(const QString& role) throw (Exception);
        static QString signalRoleToString(SignalRole_t role) noexcept;


        // Signal Attributes
        QUuid mUuid;
        SignalRole_t mRole;
        QString mForcedNetName;
        bool mIsRequired;
        bool mIsNegated;
        bool mIsClock;
        QMap<QString, QString> mNames;
        QMap<QString, QString> mDescriptions;
};

} // namespace library

#endif // LIBRARY_GENCOMPSIGNAL_H
