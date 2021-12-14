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
#include "cmdmoveselectedschematicitems.h"

#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdtextedit.h"
#include "../../project/cmd/cmdschematicnetlabelanchorsupdate.h"
#include "../../project/cmd/cmdschematicnetlabeledit.h"
#include "../../project/cmd/cmdschematicnetpointedit.h"
#include "../../project/cmd/cmdsymbolinstanceedit.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicselectionquery.h>
#include <librepcb/core/types/gridproperties.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
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
  query->addSelectedPolygons();
  query->addSelectedTexts();
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
  foreach (SI_Polygon* polygon, query->getPolygons()) {
    CmdPolygonEdit* cmd = new CmdPolygonEdit(polygon->getPolygon());
    mPolygonEditCmds.append(cmd);
  }
  foreach (SI_Text* text, query->getTexts()) {
    CmdTextEdit* cmd = new CmdTextEdit(text->getText());
    mTextEditCmds.append(cmd);
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
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdTextEdit* cmd, mTextEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
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
    qDeleteAll(mPolygonEditCmds);
    mPolygonEditCmds.clear();
    qDeleteAll(mTextEditCmds);
    mTextEditCmds.clear();
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
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    appendChild(cmd);  // can throw
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
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
}  // namespace librepcb
