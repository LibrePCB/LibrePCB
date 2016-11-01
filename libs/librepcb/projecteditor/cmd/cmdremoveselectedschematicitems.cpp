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
#include "cmdremoveselectedschematicitems.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/circuit/cmd/cmdcomponentinstanceremove.h>
#include <librepcb/project/circuit/cmd/cmdnetsignalremove.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabelremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>
#include "cmdremoveunusednetsignals.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRemoveSelectedSchematicItems::CmdRemoveSelectedSchematicItems(Schematic& schematic) noexcept :
    UndoCommandGroup(tr("Remove Schematic Elements")), mSchematic(schematic)
{
}

CmdRemoveSelectedSchematicItems::~CmdRemoveSelectedSchematicItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRemoveSelectedSchematicItems::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all selected items
    QList<SI_Base*> items = mSchematic.getSelectedItems(false, true, true, true, true, true,
                                                        true, true, true, true, false);

    // clear selection because these items will be removed now
    mSchematic.clearSelection();

    // remove all netlabels
    foreach (SI_Base* item, items) {
        if (item->getType() == SI_Base::Type_t::NetLabel) {
            SI_NetLabel* netlabel = dynamic_cast<SI_NetLabel*>(item); Q_ASSERT(netlabel);
            execNewChildCmd(new CmdSchematicNetLabelRemove(mSchematic, *netlabel)); // can throw
        }
    }

    // remove all netlines
    foreach (SI_Base* item, items) {
        if (item->getType() == SI_Base::Type_t::NetLine) {
            SI_NetLine* netline = dynamic_cast<SI_NetLine*>(item); Q_ASSERT(netline);
            execNewChildCmd(new CmdSchematicNetLineRemove(*netline)); // can throw
        }
    }

    // remove all netpoints
    foreach (SI_Base* item, items) {
        if (item->getType() == SI_Base::Type_t::NetPoint) {
            SI_NetPoint* netpoint = dynamic_cast<SI_NetPoint*>(item); Q_ASSERT(netpoint);
            if (netpoint->isAttachedToPin()) {
                detachNetPointFromSymbolPin(*netpoint); // can throw
            }
            if (netpoint->getLines().count() == 0) {
                execNewChildCmd(new CmdSchematicNetPointRemove(*netpoint)); // can throw
            }
        }
    }

    // remove all symbols, devices and component instances
    foreach (SI_Base* item, items) {
        if (item->getType() == SI_Base::Type_t::Symbol) {
            SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item); Q_ASSERT(symbol);
            execNewChildCmd(new CmdSymbolInstanceRemove(mSchematic, *symbol)); // can throw

            ComponentInstance* component = &symbol->getComponentInstance(); Q_ASSERT(component);
            if (component->getPlacedSymbolsCount() == 0) {
                foreach (Board* board, mSchematic.getProject().getBoards()) {
                    BI_Device* device = board->getDeviceInstanceByComponentUuid(component->getUuid());
                    if (device) {
                        // TODO: does not work if traces are connected to the device!
                        execNewChildCmd(new CmdDeviceInstanceRemove(*board, *device)); // can throw
                    }
                }
                execNewChildCmd(new CmdComponentInstanceRemove(mSchematic.getProject().getCircuit(),
                                                              *component)); // can throw
            }
        }
    }

    if (getChildCount() > 0) {
        // remove netsignals which are no longer required
        execNewChildCmd(new CmdRemoveUnusedNetSignals(mSchematic.getProject().getCircuit())); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

void CmdRemoveSelectedSchematicItems::detachNetPointFromSymbolPin(SI_NetPoint& netpoint) throw (Exception)
{
    SI_SymbolPin* pin = netpoint.getSymbolPin();
    if (!pin) throw LogicError(__FILE__, __LINE__);
    ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
    if (!cmpSig) throw LogicError(__FILE__, __LINE__);

    // disconnect all netlines
    QList<SI_NetLine*> netlines = netpoint.getLines();
    foreach (SI_NetLine* netline, netlines) {
        execNewChildCmd(new CmdSchematicNetLineRemove(*netline)); // can throw
    }

    // detach netpoint from symbol pin
    CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(netpoint);
    cmd->setPinToAttach(nullptr);
    execNewChildCmd(cmd); // can throw

    // reconnect all netlines
    foreach (SI_NetLine* netline, netlines) {
        execNewChildCmd(new CmdSchematicNetLineAdd(*netline)); // can throw
    }

    // check if this was the last symbol pin of the component signal
    bool componentSignalMustBeDisconnected = true;
    foreach (const SI_SymbolPin* pin, cmpSig->getRegisteredSymbolPins()) {
        if (pin->getNetPoint()) {
            componentSignalMustBeDisconnected = false;
            break;
        }
    }

    // if required, disconnect the component signal from the net signal now
    if (componentSignalMustBeDisconnected) {
        disconnectComponentSignalInstance(*cmpSig); // can throw
    }
}

void CmdRemoveSelectedSchematicItems::disconnectComponentSignalInstance(ComponentSignalInstance& signal) throw (Exception)
{
    // disconnect board items
    foreach (BI_FootprintPad* pad, signal.getRegisteredFootprintPads()) {
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) {
            foreach (BI_NetLine* netline, netpoint->getLines()) {
                execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
            }
            execNewChildCmd(new CmdBoardNetPointRemove(*netpoint)); // can throw
        }
    }

    // disconnect the component signal instance from the net signal
    execNewChildCmd(new CmdCompSigInstSetNetSignal(signal, nullptr)); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
