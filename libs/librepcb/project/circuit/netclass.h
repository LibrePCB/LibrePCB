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

#ifndef LIBREPCB_PROJECT_NETCLASS_H
#define LIBREPCB_PROJECT_NETCLASS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../erc/if_ercmsgprovider.h"
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Circuit;
class NetSignal;
class ErcMsg;

/*****************************************************************************************
 *  Class NetClass
 ****************************************************************************************/

/**
 * @brief The NetClass class
 */
class NetClass final : public QObject, public IF_ErcMsgProvider,
                       public SerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(NetClass)

    public:

        // Constructors / Destructor
        NetClass() = delete;
        NetClass(const NetClass& other) = delete;
        explicit NetClass(Circuit& circuit, const XmlDomElement& domElement) throw (Exception);
        explicit NetClass(Circuit& circuit, const QString& name) throw (Exception);
        ~NetClass() noexcept;

        // Getters
        Circuit& getCircuit() const noexcept {return mCircuit;}
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        int getNetSignalCount() const noexcept {return mRegisteredNetSignals.count();}
        bool isUsed() const noexcept {return (getNetSignalCount() > 0);}

        // Setters
        void setName(const QString& name) throw (Exception);

        // General Methods
        void addToCircuit() throw (Exception);
        void removeFromCircuit() throw (Exception);
        void registerNetSignal(NetSignal& signal) throw (Exception);
        void unregisterNetSignal(NetSignal& signal) throw (Exception);

        /// @copydoc SerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        NetClass& operator=(const NetClass& rhs) = delete;


    private:

        /// @copydoc SerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        Circuit& mCircuit;
        bool mIsAddedToCircuit;

        // Attributes
        Uuid mUuid;
        QString mName;

        // Registered Elements
        /// @brief all registered netsignals
        QHash<Uuid, NetSignal*> mRegisteredNetSignals;

        // ERC Messages
        /// @brief the ERC message for unused netclasses
        QScopedPointer<ErcMsg> mErcMsgUnusedNetClass;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_NETCLASS_H
