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

#ifndef LIBREPCB_PROJECT_NETSIGNAL_H
#define LIBREPCB_PROJECT_NETSIGNAL_H

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
class NetClass;
class ComponentSignalInstance;
class SI_NetSegment;
class BI_NetPoint;
class BI_Via;
class ErcMsg;

/*****************************************************************************************
 *  Class NetSignal
 ****************************************************************************************/

/**
 * @brief The NetSignal class
 */
class NetSignal final : public QObject, public IF_ErcMsgProvider, public SerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(NetSignal)

    public:

        // Constructors / Destructor
        NetSignal() = delete;
        NetSignal(const NetSignal& other) = delete;
        explicit NetSignal(Circuit& circuit, const SExpression& node);
        explicit NetSignal(Circuit& circuit, NetClass& netclass, const QString& name,
                           bool autoName);
        ~NetSignal() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        bool hasAutoName() const noexcept {return mHasAutoName;}
        NetClass& getNetClass() const noexcept {return *mNetClass;}
        bool isHighlighted() const noexcept {return mIsHighlighted;}

        // Getters: General
        Circuit& getCircuit() const noexcept {return mCircuit;}
        const QList<ComponentSignalInstance*>& getComponentSignals() const noexcept {return mRegisteredComponentSignals;}
        const QList<SI_NetSegment*>& getSchematicNetSegments() const noexcept {return mRegisteredSchematicNetSegments;}
        const QList<BI_NetPoint*>& getBoardNetPoints() const noexcept {return mRegisteredBoardNetPoints;}
        const QList<BI_Via*>& getBoardVias() const noexcept {return mRegisteredBoardVias;}
        int getRegisteredElementsCount() const noexcept;
        bool isUsed() const noexcept;
        bool isNameForced() const noexcept;

        // Setters
        void setName(const QString& name, bool isAutoName);
        void setHighlighted(bool hl) noexcept;

        // General Methods
        void addToCircuit();
        void removeFromCircuit();
        void registerComponentSignal(ComponentSignalInstance& signal);
        void unregisterComponentSignal(ComponentSignalInstance& signal);
        void registerSchematicNetSegment(SI_NetSegment& netsegment);
        void unregisterSchematicNetSegment(SI_NetSegment& netsegment);
        void registerBoardNetPoint(BI_NetPoint& netpoint);
        void unregisterBoardNetPoint(BI_NetPoint& netpoint);
        void registerBoardVia(BI_Via& via);
        void unregisterBoardVia(BI_Via& via);

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        NetSignal& operator=(const NetSignal& rhs) = delete;
        bool operator==(const NetSignal& rhs) noexcept {return (this == &rhs);}
        bool operator!=(const NetSignal& rhs) noexcept {return (this != &rhs);}


    signals:

        void nameChanged(const QString& newName);
        void highlightedChanged(bool isHighlighted);


    private:
        bool checkAttributesValidity() const noexcept;
        void updateErcMessages() noexcept;


        // General
        Circuit& mCircuit;
        bool mIsAddedToCircuit;
        bool mIsHighlighted;

        // Attributes
        Uuid mUuid;
        QString mName;
        bool mHasAutoName;
        NetClass* mNetClass;

        // Registered Elements of this NetSignal
        QList<ComponentSignalInstance*> mRegisteredComponentSignals;
        QList<SI_NetSegment*> mRegisteredSchematicNetSegments;
        QList<BI_NetPoint*> mRegisteredBoardNetPoints;
        QList<BI_Via*> mRegisteredBoardVias;

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
