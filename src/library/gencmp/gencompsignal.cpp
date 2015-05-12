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
#include "../../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSignal::GenCompSignal(const QUuid& uuid, const QString& name_en_US,
                             const QString& description_en_US) noexcept :
    mUuid(uuid), mRole(SignalRole_t::Passive), mForcedNetName(), mIsRequired(false),
    mIsNegated(false), mIsClock(false)
{
    Q_ASSERT(mUuid.isNull() == false);
    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
}

GenCompSignal::GenCompSignal(const XmlDomElement& domElement) throw (Exception)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mRole = stringToSignalRole(domElement.getAttribute("role"));
    mForcedNetName = domElement.getAttribute("forced_net_name");
    mIsRequired = domElement.getAttribute<bool>("required");
    mIsNegated = domElement.getAttribute<bool>("negated");
    mIsClock = domElement.getAttribute<bool>("clock");

    // read names and descriptions in all available languages
    LibraryBaseElement::readLocaleDomNodes(domElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(domElement, "description", mDescriptions);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompSignal::~GenCompSignal() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString GenCompSignal::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString GenCompSignal::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* GenCompSignal::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("signal"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("role", signalRoleToString(mRole));
    root->setAttribute("forced_net_name", mForcedNetName);
    root->setAttribute("required", mIsRequired);
    root->setAttribute("negated", mIsNegated);
    root->setAttribute("clock", mIsClock);
    foreach (const QString& locale, mNames.keys())
        root->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        root->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool GenCompSignal::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mNames.value("en_US").isEmpty())    return false;
    if (!mDescriptions.contains("en_US"))   return false;
    return true;
}

/*****************************************************************************************
 *  Private Static Methods
 ****************************************************************************************/

GenCompSignal::SignalRole_t GenCompSignal::stringToSignalRole(const QString& role) throw (Exception)
{
    if (role == "power")            return SignalRole_t::Power;
    else if (role == "input")       return SignalRole_t::Input;
    else if (role == "output")      return SignalRole_t::Output;
    else if (role == "inout")       return SignalRole_t::InOut;
    else if (role == "opendrain")   return SignalRole_t::OpenDrain;
    else if (role == "passive")     return SignalRole_t::Passive;
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
        case SignalRole_t::Power:        return "power";
        case SignalRole_t::Input:        return "input";
        case SignalRole_t::Output:       return "output";
        case SignalRole_t::InOut:        return "inout";
        case SignalRole_t::OpenDrain:    return "opendrain";
        case SignalRole_t::Passive:      return "passive";
        default:
            Q_ASSERT(false);
            qCritical() << "unknown signal role:" << static_cast<int>(role);
            return "unknown";
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
