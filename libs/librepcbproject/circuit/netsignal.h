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

#ifndef LIBREPCB_PROJECT_NETSIGNAL_H
#define LIBREPCB_PROJECT_NETSIGNAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Circuit;
class NetClass;
class ComponentSignalInstance;
class SI_NetPoint;
class SI_NetLabel;
class ErcMsg;

/*****************************************************************************************
 *  Class NetSignal
 ****************************************************************************************/

/**
 * @brief The NetSignal class
 */
class NetSignal final : public QObject, public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(NetSignal)

    public:

        // Constructors / Destructor
        NetSignal() = delete;
        NetSignal(const NetSignal& other) = delete;
        explicit NetSignal(Circuit& circuit, const XmlDomElement& domElement) throw (Exception);
        explicit NetSignal(Circuit& circuit, NetClass& netclass, const QString& name,
                           bool autoName) throw (Exception);
        ~NetSignal() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        bool hasAutoName() const noexcept {return mHasAutoName;}
        NetClass& getNetClass() const noexcept {return *mNetClass;}

        // Getters: General
        Circuit& getCircuit() const noexcept {return mCircuit;}
        const QList<ComponentSignalInstance*>& getComponentSignals() const noexcept {return mRegisteredComponentSignals;}
        const QList<SI_NetPoint*>& getNetPoints() const noexcept {return mRegisteredSchematicNetPoints;}
        const QList<SI_NetLabel*>& getNetLabels() const noexcept {return mRegisteredSchematicNetLabels;}
        int getRegisteredElementsCount() const noexcept;
        bool isUsed() const noexcept;
        bool isNameForced() const noexcept;

        // Setters
        void setName(const QString& name, bool isAutoName) throw (Exception);

        // General Methods
        void addToCircuit() throw (Exception);
        void removeFromCircuit() throw (Exception);
        void registerComponentSignal(ComponentSignalInstance& signal) throw (Exception);
        void unregisterComponentSignal(ComponentSignalInstance& signal) throw (Exception);
        void registerSchematicNetPoint(SI_NetPoint& netpoint) throw (Exception);
        void unregisterSchematicNetPoint(SI_NetPoint& netpoint) throw (Exception);
        void registerSchematicNetLabel(SI_NetLabel& netlabel) throw (Exception);
        void unregisterSchematicNetLabel(SI_NetLabel& netlabel) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        NetSignal& operator=(const NetSignal& rhs) = delete;


    signals:

        void nameChanged(const QString& newName);


    private:

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        Circuit& mCircuit;
        bool mIsAddedToCircuit;

        // Attributes
        Uuid mUuid;
        QString mName;
        bool mHasAutoName;
        NetClass* mNetClass;

        // Registered Elements of this Netclass
        QList<ComponentSignalInstance*> mRegisteredComponentSignals;
        QList<SI_NetPoint*> mRegisteredSchematicNetPoints;
        QList<SI_NetLabel*> mRegisteredSchematicNetLabels;

        // ERC Messages
        /// @brief the ERC message for unused netsignals
        QScopedPointer<ErcMsg> mErcMsgUnusedNetSignal;
        /// @brief the ERC messages for netsignals with less than two component signals
        QScopedPointer<ErcMsg> mErcMsgConnectedToLessThanTwoPins;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_NETSIGNAL_H
