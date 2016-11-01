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
#include "cmdrotateselectedschematicitems.h"
#include <librepcb/common/gridproperties.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeledit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRotateSelectedSchematicItems::CmdRotateSelectedSchematicItems(Schematic& schematic,
                                                                 const Angle& angle) noexcept :
    UndoCommandGroup(tr("Rotate Schematic Elements")), mSchematic(schematic), mAngle(angle)
{
}

CmdRotateSelectedSchematicItems::~CmdRotateSelectedSchematicItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRotateSelectedSchematicItems::performExecute() throw (Exception)
{
    // get all selected items
    QList<SI_Base*> items = mSchematic.getSelectedItems(false, true, false, true, false, false,
                                                        false, false, false, false, false);

    // no items selected --> nothing to do here
    if (items.isEmpty()) {
        return false;
    }

    // find the center of all elements
    Point center = Point(0, 0);
    foreach (SI_Base* item, items) {
        center += item->getPosition();
    }
    center /= items.count();
    center.mapToGrid(mSchematic.getGridProperties().getInterval());

    // rotate all selected elements
    foreach (SI_Base* item, items) {
        switch (item->getType())
        {
            case SI_Base::Type_t::Symbol: {
                SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item); Q_ASSERT(symbol);
                CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(*symbol);
                cmd->rotate(mAngle, center, false);
                appendChild(cmd);
                break;
            }
            case SI_Base::Type_t::NetPoint: {
                SI_NetPoint* netpoint = dynamic_cast<SI_NetPoint*>(item); Q_ASSERT(netpoint);
                CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
                cmd->setPosition(netpoint->getPosition().rotated(mAngle, center), false);
                appendChild(cmd);
                break;
            }
            case SI_Base::Type_t::NetLabel: {
                SI_NetLabel* netlabel = dynamic_cast<SI_NetLabel*>(item); Q_ASSERT(netlabel);
                CmdSchematicNetLabelEdit* cmd = new CmdSchematicNetLabelEdit(*netlabel);
                cmd->rotate(mAngle, center, false);
                appendChild(cmd);
                break;
            }
            default: {
                qCritical() << "Unknown schematic item type:" << static_cast<int>(item->getType());
                break;
            }
        }
    }

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
