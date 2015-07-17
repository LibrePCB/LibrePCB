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

#ifndef PROJECT_NETCLASS_H
#define PROJECT_NETCLASS_H

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
class NetSignal;
class ErcMsg;
}

/*****************************************************************************************
 *  Class NetClass
 ****************************************************************************************/

namespace project {

/**
 * @brief The NetClass class
 */
class NetClass final : public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(NetClass)
        DECLARE_ERC_MSG_CLASS_NAME(NetClass)

    public:

        // Constructors / Destructor
        explicit NetClass(const Circuit& circuit, const XmlDomElement& domElement) throw (Exception);
        explicit NetClass(const Circuit& circuit, const QString& name) throw (Exception);
        ~NetClass() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        int getNetSignalCount() const noexcept {return mNetSignals.count();}

        // Setters
        void setName(const QString& name) throw (Exception);

        // NetSignal Methods
        void registerNetSignal(NetSignal& signal) noexcept;
        void unregisterNetSignal(NetSignal& signal) noexcept;

        // General Methods
        void addToCircuit() noexcept;
        void removeFromCircuit() noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement(int version) const throw (Exception) override;


    private:

        // make some methods inaccessible...
        NetClass();
        NetClass(const NetClass& other);
        NetClass& operator=(const NetClass& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        const Circuit& mCircuit;
        bool mAddedToCircuit;

        // Misc
        /// @brief the ERC message for unused netclasses
        ErcMsg* mErcMsgUnusedNetClass;
        /// @brief all registered netsignals
        QHash<QUuid, NetSignal*> mNetSignals;

        // Attributes
        QUuid mUuid;
        QString mName;
};

} // namespace project

#endif // PROJECT_NETCLASS_H
