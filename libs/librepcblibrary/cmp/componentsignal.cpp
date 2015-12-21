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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "componentsignal.h"
#include "component.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSignal::ComponentSignal(const Uuid& uuid, const QString& name) noexcept :
    mUuid(uuid), mName(name), mRole(SignalRole_t::Passive), mForcedNetName(),
    mIsRequired(false), mIsNegated(false), mIsClock(false)
{
    Q_ASSERT(mUuid.isNull() == false);
}

ComponentSignal::ComponentSignal(const XmlDomElement& domElement) throw (Exception)
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid");
    mName = domElement.getText(true);
    mRole = stringToSignalRole(domElement.getAttribute("role"));
    mForcedNetName = domElement.getAttribute("forced_net_name");
    mIsRequired = domElement.getAttribute<bool>("required");
    mIsNegated = domElement.getAttribute<bool>("negated");
    mIsClock = domElement.getAttribute<bool>("clock");

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentSignal::~ComponentSignal() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* ComponentSignal::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("signal"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("role", signalRoleToString(mRole));
    root->setAttribute("forced_net_name", mForcedNetName);
    root->setAttribute("required", mIsRequired);
    root->setAttribute("negated", mIsNegated);
    root->setAttribute("clock", mIsClock);
    root->setText(mName);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSignal::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  Private Static Methods
 ****************************************************************************************/

ComponentSignal::SignalRole_t ComponentSignal::stringToSignalRole(const QString& role) throw (Exception)
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

QString ComponentSignal::signalRoleToString(SignalRole_t role) noexcept
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
