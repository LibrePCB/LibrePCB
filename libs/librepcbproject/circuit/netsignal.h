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

#ifndef PROJECT_NETSIGNAL_H
#define PROJECT_NETSIGNAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class NetClass;
class GenCompSignalInstance;
class SI_NetPoint;
class SI_NetLabel;
class ErcMsg;
}

/*****************************************************************************************
 *  Class NetSignal
 ****************************************************************************************/

namespace project {

/**
 * @brief The NetSignal class
 */
class NetSignal final : public QObject, public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(NetSignal)

    public:

        // Constructors / Destructor
        explicit NetSignal(const Circuit& circuit,
                           const XmlDomElement& domElement) throw (Exception);
        explicit NetSignal(const Circuit& circuit, NetClass& netclass,
                           const QString& name, bool autoName) throw (Exception);
        ~NetSignal() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        bool hasAutoName() const noexcept {return mHasAutoName;}
        NetClass& getNetClass() const noexcept {return *mNetClass;}
        bool isNameForced() const noexcept {return (mGenCompSignalWithForcedNameCount > 0);}
        const QList<GenCompSignalInstance*>& getGenCompSignals() const noexcept {return mGenCompSignals;}
        const QList<SI_NetPoint*>& getNetPoints() const noexcept {return mSchematicNetPoints;}
        const QList<SI_NetLabel*>& getNetLabels() const noexcept {return mSchematicNetLabels;}

        // Setters
        void setName(const QString& name, bool isAutoName) throw (Exception);

        // General Methods
        void registerGenCompSignal(GenCompSignalInstance& signal) noexcept;
        void unregisterGenCompSignal(GenCompSignalInstance& signal) noexcept;
        void registerSchematicNetPoint(SI_NetPoint& netpoint) noexcept;
        void unregisterSchematicNetPoint(SI_NetPoint& netpoint) noexcept;
        void registerSchematicNetLabel(SI_NetLabel& netlabel) noexcept;
        void unregisterSchematicNetLabel(SI_NetLabel& netlabel) noexcept;
        void addToCircuit() noexcept;
        void removeFromCircuit() noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    signals:

        void nameChanged(const QString& newName);


    private:

        // make some methods inaccessible...
        NetSignal();
        NetSignal(const NetSignal& other);
        NetSignal& operator=(const NetSignal& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        const Circuit& mCircuit;
        bool mAddedToCircuit;

        // Misc

        /// @brief the ERC message for unused netsignals
        ErcMsg* mErcMsgUnusedNetSignal;
        /// @brief the ERC messages for netsignals with less than two generic component signals
        ErcMsg* mErcMsgConnectedToLessThanTwoPins;

        // Registered Elements of this Netclass
        QList<GenCompSignalInstance*> mGenCompSignals;
        QList<SI_NetPoint*> mSchematicNetPoints;
        QList<SI_NetLabel*> mSchematicNetLabels;
        int mGenCompSignalWithForcedNameCount;

        // Attributes
        QUuid mUuid;
        QString mName;
        bool mHasAutoName;
        NetClass* mNetClass;
};

} // namespace project

#endif // PROJECT_NETSIGNAL_H
