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

#ifndef PROJECT_GENERICCOMPONENTINSTANCE_H
#define PROJECT_GENERICCOMPONENTINSTANCE_H

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
class GenCompSignalInstance;
}

namespace library {
class GenericComponent;
}

/*****************************************************************************************
 *  Class GenericComponentInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenericComponentInstance class
 */
class GenericComponentInstance : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenericComponentInstance(Circuit& circuit, const QDomElement& domElement)
                                          throw (Exception);
        ~GenericComponentInstance() noexcept;

        // Getters
        const Circuit& getCircuit() const noexcept {return mCircuit;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        const library::GenericComponent& getGenComp() const noexcept {return *mGenComp;}

        // Setters
        void setName(const QString& name) throw (Exception);

        // General Methods
        void addToCircuit(bool addNode, QDomElement& parent) throw (Exception);
        void removeFromCircuit(bool removeNode, QDomElement& parent) throw (Exception);

        // Static Methods
        static GenericComponentInstance* create(Circuit& circuit, QDomDocument& doc,
                                                const QUuid& genericComponent,
                                                const QString& name) throw (Exception);

    private:

        // make some methods inaccessible...
        GenericComponentInstance();
        GenericComponentInstance(const GenericComponentInstance& other);
        GenericComponentInstance& operator=(const GenericComponentInstance& rhs);

        // General
        Circuit& mCircuit;
        QDomElement mDomElement;

        // Attributes
        QUuid mUuid;
        QString mName;
        const library::GenericComponent* mGenComp;
        QHash<QUuid, GenCompSignalInstance*> mSignals;
};

} // namespace project

#endif // PROJECT_GENERICCOMPONENTINSTANCE_H
