/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QDomElement>
#include "../erc/if_ercmsgprovider.h"
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class NetClass;
class GenCompSignalInstance;
class SchematicNetPoint;
class ErcMsg;
}

/*****************************************************************************************
 *  Class NetSignal
 ****************************************************************************************/

namespace project {

/**
 * @brief The NetSignal class
 */
class NetSignal final : public QObject, public IF_ErcMsgProvider
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(NetSignal)

    public:

        // Constructors / Destructor
        explicit NetSignal(Circuit& circuit, const QDomElement& domElement) throw (Exception);
        ~NetSignal() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        bool hasAutoName() const noexcept {return mAutoName;}
        NetClass& getNetClass() const noexcept {return *mNetClass;}
        bool isNameForced() const noexcept {return (mGenCompSignalWithForcedNameCount > 0);}
        const QList<GenCompSignalInstance*>& getGenCompSignals() const noexcept {return mGenCompSignals;}
        const QList<SchematicNetPoint*>& getNetPoints() const noexcept {return mSchematicNetPoints;}

        // Setters
        void setName(const QString& name, bool isAutoName) throw (Exception);

        // General Methods
        void registerGenCompSignal(GenCompSignalInstance* signal) noexcept;
        void unregisterGenCompSignal(GenCompSignalInstance* signal) noexcept;
        void registerSchematicNetPoint(SchematicNetPoint* netpoint) noexcept;
        void unregisterSchematicNetPoint(SchematicNetPoint* netpoint) noexcept;
        void addToCircuit(bool addNode, QDomElement& parent) throw (Exception);
        void removeFromCircuit(bool removeNode, QDomElement& parent) throw (Exception);

        // Static Methods
        static NetSignal* create(Circuit& circuit, QDomDocument& doc,
                                 const QUuid& netclass, const QString& name,
                                 bool autoName) throw (Exception);

    private:

        // make some methods inaccessible...
        NetSignal();
        NetSignal(const NetSignal& other);
        NetSignal& operator=(const NetSignal& rhs);

        // Private Methods
        void updateErcMessages() noexcept;

        // General
        Circuit& mCircuit;
        QDomElement mDomElement;
        bool mAddedToCircuit;

        // Attributes
        QUuid mUuid;
        QString mName;
        bool mAutoName;
        NetClass* mNetClass;

        // Misc

        /// @brief the ERC message for unused netsignals
        QScopedPointer<ErcMsg> mErcMsgUnusedNetSignal;
        /// @brief the ERC messages for netsignals with less than two generic component signals
        QScopedPointer<ErcMsg> mErcMsgConnectedToLessThanTwoPins;

        // Registered Elements of this Netclass
        QList<GenCompSignalInstance*> mGenCompSignals;
        QList<SchematicNetPoint*> mSchematicNetPoints;
        uint mGenCompSignalWithForcedNameCount;
};

} // namespace project

#endif // PROJECT_NETSIGNAL_H
