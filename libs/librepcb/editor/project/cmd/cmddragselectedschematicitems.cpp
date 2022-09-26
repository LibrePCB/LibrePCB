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
#include "cmddragselectedschematicitems.h"

#include "../../project/cmd/cmdschematicnetlabelanchorsupdate.h"
#include "../../project/cmd/cmdschematicnetlabeledit.h"
#include "../../project/cmd/cmdschematicnetpointedit.h"
#include "../../project/cmd/cmdsymbolinstanceedit.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
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

CmdDragSelectedSchematicItems::CmdDragSelectedSchematicItems(
    Schematic& schematic, const Point& startPos) noexcept
  : UndoCommandGroup(tr("Drag Schematic Elements")),
    mSchematic(schematic),
    mStartPos(startPos),
    mDeltaPos(0, 0),
    mCenterPos(0, 0),
    mDeltaAngle(0),
    mMirrored(false) {
  // get all selected items
  std::unique_ptr<SchematicSelectionQuery> query(
      mSchematic.createSelectionQuery());
  query->addSelectedSymbols();
  query->addSelectedNetPoints();
  query->addSelectedNetLines();
  query->addSelectedNetLabels();
  query->addNetPointsOfNetLines();

  // Find the center of all elements and create undo commands.
  int count = 0;
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    mCenterPos += symbol->getPosition();
    ++count;
    CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(*symbol);
    mSymbolEditCmds.append(cmd);
  }
  foreach (SI_NetPoint* netpoint, query->getNetPoints()) {
    mCenterPos += netpoint->getPosition();
    ++count;
    CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
    mNetPointEditCmds.append(cmd);
  }
  foreach (SI_NetLabel* netlabel, query->getNetLabels()) {
    mCenterPos += netlabel->getPosition();
    ++count;
    CmdSchematicNetLabelEdit* cmd = new CmdSchematicNetLabelEdit(*netlabel);
    mNetLabelEditCmds.append(cmd);
  }

  if (count > 0) {
    mCenterPos /= count;
    mCenterPos.mapToGrid(mSchematic.getGridProperties().getInterval());
  }
}

CmdDragSelectedSchematicItems::~CmdDragSelectedSchematicItems() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedSchematicItems::setCurrentPosition(
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
    mDeltaPos = delta;
  }
}

void CmdDragSelectedSchematicItems::rotate(const Angle& angle,
                                           bool aroundItemsCenter) noexcept {
  Point center = (aroundItemsCenter ? mCenterPos : mStartPos) + mDeltaPos;

  // rotate selected elements
  foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
    cmd->rotate(angle, center, true);
  }
  mDeltaAngle += angle;
}

void CmdDragSelectedSchematicItems::mirror(Qt::Orientation orientation,
                                           bool aroundItemsCenter) noexcept {
  Point center = (aroundItemsCenter ? mCenterPos : mStartPos) + mDeltaPos;

  // rotate selected elements
  foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
    cmd->mirror(center, orientation, true);
  }
  foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
    Point newpos =
        cmd->getNetLabel().getPosition().mirrored(orientation, center);

    // Compensate offset only for horizontal positioning
    // Since there is no right alignment (yet), coordinates need to be
    // re-adjusted to accommodate left shift.
    // New position = mirrored old position - label width
    Angle labelRotation = cmd->getNetLabel().getRotation().mappedTo0_360deg();
    if (labelRotation == Angle::deg0()) {
      newpos.setX(newpos.getX() - cmd->getNetLabel().getApproximateWidth());
      newpos.mapToGrid(mSchematic.getGridProperties().getInterval());
    } else if (labelRotation == Angle::deg180()) {
      newpos.setX(newpos.getX() + cmd->getNetLabel().getApproximateWidth());
      newpos.mapToGrid(mSchematic.getGridProperties().getInterval());
    }

    cmd->setPosition(newpos, false);
  }
  mMirrored = !mMirrored;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedSchematicItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaAngle == Angle::deg0()) && (!mMirrored)) {
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
}  // namespace librepcb
