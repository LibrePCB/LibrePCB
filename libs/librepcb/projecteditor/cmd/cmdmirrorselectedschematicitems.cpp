/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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
#include "cmdmirrorselectedschematicitems.h"

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

CmdMirrorSelectedSchematicItems::CmdMirrorSelectedSchematicItems(
    Schematic& schematic, Qt::Orientation orientation) noexcept
  : UndoCommandGroup(tr("Mirror Schematic Elements")),
    mSchematic(schematic),
    mOrientation(orientation) {
}

CmdMirrorSelectedSchematicItems::~CmdMirrorSelectedSchematicItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdMirrorSelectedSchematicItems::performExecute() {
  // get all selected items
  std::unique_ptr<SchematicSelectionQuery> query(
      mSchematic.createSelectionQuery());
  query->addSelectedSymbols();
  query->addSelectedNetPoints();
  query->addNetPointsOfNetLines();
  query->addSelectedNetLabels();

  // find the center of all elements
  Point center = Point(0, 0);
  int   count  = 0;
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    center += symbol->getPosition();
    ++count;
  }
  foreach (SI_NetPoint* netpoint, query->getNetPoints()) {
    center += netpoint->getPosition();
    ++count;
  }
  foreach (SI_NetLabel* netlabel, query->getNetLabels()) {
    center += netlabel->getPosition();
    ++count;
  }
  if (count > 0) {
    center /= count;
    center.mapToGrid(mSchematic.getGridProperties().getInterval());
  } else {
    // no items selected --> nothing to do here
    return false;
  }

  // mirror all selected elements
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(*symbol);
    cmd->mirror(center, mOrientation, false);
    appendChild(cmd);
  }
  foreach (SI_NetPoint* netpoint, query->getNetPoints()) {
    CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
    cmd->setPosition(netpoint->getPosition().mirrored(mOrientation, center),
                     false);
    appendChild(cmd);
  }
  foreach (SI_NetLabel* netlabel, query->getNetLabels()) {
    // Since there is no right alignment (yet), coordinates need to be re-adjusted to accomodate left shift
    // New position = mirrored old position - label width
    Point newpos = netlabel->getPosition().mirrored(mOrientation, center);
    newpos.setX(newpos.getX() - netlabel->getApproximateWidth());
    CmdSchematicNetLabelEdit* cmd = new CmdSchematicNetLabelEdit(*netlabel);
    cmd->setPosition(newpos, false);
    appendChild(cmd);
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
