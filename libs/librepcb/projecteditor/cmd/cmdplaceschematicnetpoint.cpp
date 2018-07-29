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
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netclass.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/circuit/cmd/cmdnetclassadd.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaladd.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremoveelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include "cmdcombinenetsignals.h"
#include "cmdcombineschematicnetpoints.h"
#include "cmdcombineallitemsunderschematicnetpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdPlaceSchematicNetPoint::CmdPlaceSchematicNetPoint(Schematic& schematic, const Point& pos) noexcept :
    UndoCommandGroup(tr("Place Schematic Netpoint")),
    mCircuit(schematic.getProject().getCircuit()), mSchematic(schematic), mPosition(pos),
    mNetPoint(nullptr)
{
}

CmdPlaceSchematicNetPoint::~CmdPlaceSchematicNetPoint() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdPlaceSchematicNetPoint::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all netpoints at the specified position
    QList<SI_NetPoint*> netpointsUnderCursor = mSchematic.getNetPointsAtScenePos(mPosition);

    // determine whether we have to create a new netpoint or not
    if (netpointsUnderCursor.isEmpty()) {
        NetSignal& netsignal = createNewNetSignal(); // can throw
        SI_NetSegment& netsegment = createNewNetSegment(netsignal); // can throw
        mNetPoint = &createNewNetPoint(netsegment); // can throw
    } else {
        mNetPoint = netpointsUnderCursor.first();
    }
    Q_ASSERT(mNetPoint);

    // merge all schematic items under the resulting netpoint together
    execNewChildCmd(new CmdCombineAllItemsUnderSchematicNetPoint(*mNetPoint)); // can throw

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

NetSignal& CmdPlaceSchematicNetPoint::createNewNetSignal()
{
    // get or add netclass with the name "default"
    NetClass* netclass = mCircuit.getNetClassByName(ElementName("default"));
    if (!netclass) {
        CmdNetClassAdd* cmd = new CmdNetClassAdd(mCircuit, ElementName("default"));
        execNewChildCmd(cmd); // can throw
        netclass = cmd->getNetClass();
    }
    Q_ASSERT(netclass);

    // add new netsignal
    CmdNetSignalAdd* cmd = new CmdNetSignalAdd(mCircuit, *netclass); // auto-name
    execNewChildCmd(cmd); // can throw
    NetSignal* netsignal = cmd->getNetSignal(); Q_ASSERT(netsignal);
    return *netsignal;
}

SI_NetSegment& CmdPlaceSchematicNetPoint::createNewNetSegment(NetSignal& netsignal)
{
    CmdSchematicNetSegmentAdd* cmd = new CmdSchematicNetSegmentAdd(mSchematic, netsignal);
    execNewChildCmd(cmd); // can throw
    SI_NetSegment* netsegment = cmd->getNetSegment(); Q_ASSERT(netsegment);
    return *netsegment;
}

SI_NetPoint& CmdPlaceSchematicNetPoint::createNewNetPoint(SI_NetSegment& netsegment)
{
    CmdSchematicNetSegmentAddElements* cmd = new CmdSchematicNetSegmentAddElements(netsegment);
    SI_NetPoint* netpoint = cmd->addNetPoint(mPosition); Q_ASSERT(netpoint);
    execNewChildCmd(cmd); // can throw
    return *netpoint;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
