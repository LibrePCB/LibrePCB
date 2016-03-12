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
#include "cmdcompsiginstsetnetsignal.h"
#include "../componentsignalinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCompSigInstSetNetSignal::CmdCompSigInstSetNetSignal(ComponentSignalInstance& cmpSigInstance,
                                                       NetSignal* netsignal) noexcept :
    UndoCommand(tr("Change component signal net")),
    mComponentSignalInstance(cmpSigInstance), mNetSignal(netsignal),
    mOldNetSignal(cmpSigInstance.getNetSignal())
{
}

CmdCompSigInstSetNetSignal::~CmdCompSigInstSetNetSignal() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdCompSigInstSetNetSignal::performExecute() throw (Exception)
{
    performRedo(); // can throw
}

void CmdCompSigInstSetNetSignal::performUndo() throw (Exception)
{
    mComponentSignalInstance.setNetSignal(mOldNetSignal); // can throw
}

void CmdCompSigInstSetNetSignal::performRedo() throw (Exception)
{
    mComponentSignalInstance.setNetSignal(mNetSignal); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
