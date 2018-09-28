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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdmoveselectedschematicitems.h"

#include <librepcb/common/gridproperties.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabelanchorsupdate.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeledit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/schematicselectionquery.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdMoveSelectedSchematicItems::CmdMoveSelectedSchematicItems(
    Schematic& schematic, const Point& startPos) noexcept
  : UndoCommandGroup(tr("Move Schematic Elements")),
    mSchematic(schematic),
    mStartPos(startPos),
    mDeltaPos(0, 0) {
  // get all selected items
  std::unique_ptr<SchematicSelectionQuery> query(
      mSchematic.createSelectionQuery());
  query->addSelectedSymbols();
  query->addSelectedNetPoints();
  query->addSelectedNetLines();
  query->addSelectedNetLabels();
  query->addNetPointsOfNetLines();

  // create undo commands
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(*symbol);
    mSymbolEditCmds.append(cmd);
  }
  foreach (SI_NetPoint* netpoint, query->getNetPoints()) {
    CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
    mNetPointEditCmds.append(cmd);
  }
  foreach (SI_NetLabel* netlabel, query->getNetLabels()) {
    CmdSchematicNetLabelEdit* cmd = new CmdSchematicNetLabelEdit(*netlabel);
    mNetLabelEditCmds.append(cmd);
  }
}

CmdMoveSelectedSchematicItems::~CmdMoveSelectedSchematicItems() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdMoveSelectedSchematicItems::setCurrentPosition(
    const Point& pos) noexcept {
  Point delta = pos - mStartPos;
  delta.mapToGrid(mSchematic.getGridProperties().getInterval());

  if (delta != mDeltaPos) {
    // move selected elements
    foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    mDeltaPos = delta;
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdMoveSelectedSchematicItems::performExecute() {
  if (mDeltaPos.isOrigin()) {
    // no movement required --> discard all move commands
    qDeleteAll(mSymbolEditCmds);
    mSymbolEditCmds.clear();
    qDeleteAll(mNetPointEditCmds);
    mNetPointEditCmds.clear();
    qDeleteAll(mNetLabelEditCmds);
    mNetLabelEditCmds.clear();
    return false;
  }

  foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
    appendChild(cmd);  // can throw
  }

  // if something was modified, trigger anchors update of all netlabels
  if (getChildCount() > 0) {
    appendChild(new CmdSchematicNetLabelAnchorsUpdate(mSchematic));
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
