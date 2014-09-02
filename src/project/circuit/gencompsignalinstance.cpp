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

#include <QtCore>
#include <QDomElement>
#include "../../common/exceptions.h"
#include "gencompsignalinstance.h"
#include "genericcomponentinstance.h"
#include "circuit.h"
#include "netsignal.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSignalInstance::GenCompSignalInstance(GenericComponentInstance& genCompInstance,
                                             const QDomElement& domElement) throw (Exception) :
    QObject(0), mGenCompInstance(genCompInstance), mDomElement(domElement), mNetSignal(0),
    mAddedToCircuit(false)
{
    mCompSignalUuid = mDomElement.attribute("comp_signal");
    if(mCompSignalUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("comp_signal"),
            QString(tr("Invalid component signal UUID: \"%1\""))
            .arg(mDomElement.attribute("comp_signal")));
    }

    QString netsignalUuid = mDomElement.attribute("netsignal");
    if (!netsignalUuid.isEmpty())
    {
        mNetSignal = mGenCompInstance.getCircuit().getNetSignalByUuid(netsignalUuid);
        if(!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, netsignalUuid,
                QString(tr("Invalid netsignal UUID: \"%1\"")).arg(netsignalUuid));
        }
    }
}

GenCompSignalInstance::~GenCompSignalInstance() noexcept
{
    if (mAddedToCircuit)
        qWarning() << "generic component signal instance is still added to circuit!";
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenCompSignalInstance::addToCircuit() noexcept
{
    if (mNetSignal)
        mNetSignal->registerGenCompSignal(this);

    mAddedToCircuit = true;
}

void GenCompSignalInstance::removeFromCircuit() noexcept
{
    if (mNetSignal)
        mNetSignal->unregisterGenCompSignal(this);

    mAddedToCircuit = false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
