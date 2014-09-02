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
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class NetClass;
class GenCompSignalInstance;
class SchematicNetPoint;
}

/*****************************************************************************************
 *  Class NetSignal
 ****************************************************************************************/

namespace project {

/**
 * @brief The NetSignal class
 */
class NetSignal final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit NetSignal(Circuit& circuit, const QDomElement& domElement) throw (Exception);
        ~NetSignal() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        bool hasAutoName() const noexcept {return mAutoName;}
        NetClass& getNetClass() const noexcept {return *mNetClass;}
        int getGenCompSignalCount() const noexcept {return mGenCompSignals.count();}
        int getNetPointCount() const noexcept {return mSchematicNetPoints.count();}

        // Setters
        void setName(const QString& name) throw (Exception);

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

        // General
        Circuit& mCircuit;
        QDomElement mDomElement;

        // Attributes
        QUuid mUuid;
        QString mName;
        bool mAutoName;
        NetClass* mNetClass;

        // Registered Elements of this Netclass
        QList<GenCompSignalInstance*> mGenCompSignals;
        QList<SchematicNetPoint*> mSchematicNetPoints;
};

} // namespace project

#endif // PROJECT_NETSIGNAL_H
