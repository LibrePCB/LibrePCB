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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdplaceschematicnetpoint.h"
#include <librepcbcommon/scopeguard.h>
#include <librepcbproject/project.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/circuit/netclass.h>
#include <librepcbproject/circuit/netsignal.h>
#include <librepcbproject/circuit/componentsignalinstance.h>
#include <librepcbproject/schematics/schematic.h>
#include <librepcbproject/schematics/items/si_netpoint.h>
#include <librepcbproject/schematics/items/si_netline.h>
#include <librepcbproject/schematics/items/si_symbolpin.h>
#include <librepcbproject/circuit/cmd/cmdnetclassadd.h>
#include <librepcbproject/circuit/cmd/cmdnetsignaladd.h>
#include <librepcbproject/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineremove.h>
#include "cmdcombinenetsignals.h"
#include "cmdcombineschematicnetpoints.h"
#include "cmdcombineallnetsignalsunderschematicnetpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdPlaceSchematicNetPoint::CmdPlaceSchematicNetPoint(Schematic& schematic,
        const Point& pos, const QString& netclass, const QString& netsignal) noexcept :
    UndoCommandGroup(tr("Place Schematic Netpoint")),
    mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic), mPosition(pos),
    mNetClassName(netclass),  mNetSignalName(netsignal), mNetPoint(nullptr)
{
}

CmdPlaceSchematicNetPoint::~CmdPlaceSchematicNetPoint() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdPlaceSchematicNetPoint::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all netpoints at the specified position
    QList<SI_NetPoint*> netpointsUnderCursor = mSchematic.getNetPointsAtScenePos(mPosition);

    // determine whether we have to create a new netpoint or not
    if (netpointsUnderCursor.isEmpty()) {
        NetSignal* netsignal = getOrCreateNewNetSignal(); // can throw
        mNetPoint = createNewNetPoint(*netsignal);
    } else {
        mNetPoint = netpointsUnderCursor.first();
    }
    Q_ASSERT(mNetPoint);

    // merge all net items under the resulting netpoint together
    execNewChildCmd(new CmdCombineAllNetSignalsUnderSchematicNetPoint(*mNetPoint)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

NetSignal* CmdPlaceSchematicNetPoint::getOrCreateNewNetSignal() throw (Exception)
{
    NetSignal* netsignal = mCircuit.getNetSignalByName(mNetSignalName);
    if (netsignal) {
        return netsignal;
    } else {
        NetClass* netclass = mCircuit.getNetClassByName(mNetClassName);
        if (!netclass) {
            // add new netclass
            CmdNetClassAdd* cmd = new CmdNetClassAdd(mCircuit, mNetClassName);
            execNewChildCmd(cmd); // can throw
            netclass = cmd->getNetClass();
        }
        Q_ASSERT(netclass);

        // add new netsignal
        CmdNetSignalAdd* cmd = nullptr;
        if (mNetSignalName.isEmpty()) {
            cmd = new CmdNetSignalAdd(mCircuit, *netclass); // auto-name
        } else {
            cmd = new CmdNetSignalAdd(mCircuit, *netclass, mNetSignalName);
        }
        execNewChildCmd(cmd); // can throw
        return cmd->getNetSignal();
    }
}

SI_NetPoint* CmdPlaceSchematicNetPoint::createNewNetPoint(NetSignal& netsignal) throw (Exception)
{
    CmdSchematicNetPointAdd* cmd = new CmdSchematicNetPointAdd(mSchematic, netsignal, mPosition);
    execNewChildCmd(cmd); // can throw
    return cmd->getNetPoint();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
