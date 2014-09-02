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

#ifndef PROJECT_GENCOMPSIGNALINSTANCE_H
#define PROJECT_GENCOMPSIGNALINSTANCE_H

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
class GenericComponentInstance;
class NetSignal;
}

/*****************************************************************************************
 *  Class GenCompSignalInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenCompSignalInstance class
 */
class GenCompSignalInstance final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenCompSignalInstance(GenericComponentInstance& genCompInstance,
                                       const QDomElement& domElement) throw (Exception);
        ~GenCompSignalInstance() noexcept;

        // Getters
        const QUuid& getCompSignalUuid() const noexcept {return mCompSignalUuid;}
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}

        // General Methods
        void addToCircuit() noexcept;
        void removeFromCircuit() noexcept;

    private:

        // make some methods inaccessible...
        GenCompSignalInstance();
        GenCompSignalInstance(const GenCompSignalInstance& other);
        GenCompSignalInstance& operator=(const GenCompSignalInstance& rhs);

        // General
        GenericComponentInstance& mGenCompInstance;
        QDomElement mDomElement;

        // Attributes
        QUuid mCompSignalUuid; ///< @todo replace this with a pointer to the component signal object
        NetSignal* mNetSignal;

        bool mAddedToCircuit;

};

} // namespace project

#endif // PROJECT_GENCOMPSIGNALINSTANCE_H
