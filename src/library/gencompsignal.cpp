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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "gencompsignal.h"
#include "genericcomponent.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSignal::GenCompSignal(GenericComponent& genComp,
                             const QDomElement& domElement) throw (Exception) :
    QObject(0), mGenericComponent(genComp), mDomElement(domElement)
{
    // read UUID
    mUuid = mDomElement.attribute("uuid");
    if (mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mGenericComponent.getXmlFilepath().toStr(),
            QString(tr("Invalid signal UUID in file \"%1\"."))
            .arg(mGenericComponent.getXmlFilepath().toNative()));
    }

    // read signal attributes
    mRole = stringToSignalRole(mDomElement.attribute("role"));
    mIsRequired = (mDomElement.attribute("required") == "true");
    mIsNegated = (mDomElement.attribute("negated") == "true");
    mIsClock = (mDomElement.attribute("clock") == "true");

    // read names and descriptions in all available languages
    LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), mDomElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), mDomElement, "description", mDescriptions);
}

GenCompSignal::~GenCompSignal() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString GenCompSignal::getName(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, locale);
}

QString GenCompSignal::getDescription(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, locale);
}

/*****************************************************************************************
 *  Private Static Methods
 ****************************************************************************************/

GenCompSignal::SignalRole_t GenCompSignal::stringToSignalRole(const QString& role) throw (Exception)
{
    if (role == "power") return Role_Power;
    else if (role == "input") return Role_Input;
    else if (role == "output") return Role_Output;
    else if (role == "inout") return Role_InOut;
    else if (role == "opendrain") return Role_OpenDrain;
    else if (role == "passive") return Role_Passive;
    else if (role == "supply") return Role_Supply;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, role,
            QString(tr("Invalid signal role: \"%1\"")).arg(role));
    }
}

QString GenCompSignal::signalRoleToString(SignalRole_t role) noexcept
{
    switch (role)
    {
        case Role_Power:        return "power";
        case Role_Input:        return "input";
        case Role_Output:       return "output";
        case Role_InOut:        return "inout";
        case Role_OpenDrain:    return "opendrain";
        case Role_Passive:      return "passive";
        case Role_Supply:       return "supply";
        default:
            qCritical() << "unknown signal role:" << role;
            return "passive";
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
