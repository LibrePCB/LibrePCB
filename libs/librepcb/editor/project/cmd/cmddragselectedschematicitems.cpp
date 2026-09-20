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

// Note: This file was modified with the assistance of Claude AI.
// All modifications were reviewed by Avetos Design on 2026-09-19.

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmddragselectedschematicitems.h"

#include "../../cmd/cmdimageedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdtextedit.h"
#include "../../project/cmd/cmdschematicbusjunctionedit.h"
#include "../../project/cmd/cmdschematicbuslabeledit.h"
#include "../../project/cmd/cmdschematicnetlabeledit.h"
#include "../../project/cmd/cmdschematicnetpointedit.h"
#include "../../project/cmd/cmdsymbolinstanceedit.h"
#include "../../project/cmd/cmdsymbolinstancetextsreset.h"
#include "../schematic/schematicgraphicsscene.h"
#include "../schematic/schematicselectionquery.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/project/schematic/items/si_image.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>

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
    SchematicGraphicsScene& scene, const Point& startPos) noexcept
  : UndoCommandGroup(tr("Drag Schematic Elements")),
    mSchematic(scene.getSchematic()),
    mItemCount(0),
    mStartPos(startPos),
    mDeltaPos(0, 0),
    mCenterPos(0, 0),
    mDeltaAngle(0),
    mSnappedToGrid(false),
    mMirrored(false),
    mTextsReset(false) {
  // get all selected items
  SchematicSelectionQuery query(scene);
  query.addSelectedSymbols();
  query.addSelectedBusLines();
  query.addSelectedBusJunctions();
  query.addSelectedBusLabels();
  query.addSelectedNetPoints();
  query.addSelectedNetLines();
  query.addSelectedNetLabels();
  query.addSelectedPolygons();
  query.addSelectedSchematicTexts();
  query.addSelectedSymbolTexts();
  query.addSelectedImages();
  query.addJunctionsOfBusLines();
  query.addNetPointsOfNetLines();

  // Remember all segments whose geometry is affected by the drag. For symbols,
  // the connected net lines are not part of the selection query, so collect
  // their segments explicitly through the pins.
  for (SI_BusLine* line : query.getBusLines()) {
    mModifiedBusSegments.insert(&line->getBusSegment());
  }
  for (SI_BusJunction* junction : query.getBusJunctions()) {
    mModifiedBusSegments.insert(&junction->getBusSegment());
  }
  for (SI_NetLine* line : query.getNetLines()) {
    mModifiedNetSegments.insert(&line->getNetSegment());
  }
  for (SI_NetPoint* point : query.getNetPoints()) {
    mModifiedNetSegments.insert(&point->getNetSegment());
  }
  for (SI_Symbol* symbol : query.getSymbols()) {
    for (SI_SymbolPin* pin : symbol->getPins()) {
      if (SI_NetSegment* segment = pin->getNetSegmentOfLines()) {
        mModifiedNetSegments.insert(segment);
      }
    }
  }

  // Find the center of all elements and create undo commands.
  foreach (SI_Symbol* symbol, query.getSymbols()) {
    mCenterPos += symbol->getPosition();
    ++mItemCount;
    CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(*symbol);
    mSymbolEditCmds.append(cmd);
    mSymbolTextsResetCmds.append(new CmdSymbolInstanceTextsReset(*symbol));
  }
  foreach (SI_BusJunction* junction, query.getBusJunctions()) {
    mCenterPos += junction->getPosition();
    ++mItemCount;
    CmdSchematicBusJunctionEdit* cmd =
        new CmdSchematicBusJunctionEdit(*junction);
    mBusJunctionEditCmds.append(cmd);
  }
  foreach (SI_BusLabel* label, query.getBusLabels()) {
    mCenterPos += label->getPosition();
    ++mItemCount;
    CmdSchematicBusLabelEdit* cmd = new CmdSchematicBusLabelEdit(*label);
    mBusLabelEditCmds.append(cmd);
  }
  foreach (SI_NetPoint* netpoint, query.getNetPoints()) {
    mCenterPos += netpoint->getPosition();
    ++mItemCount;
    CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
    mNetPointEditCmds.append(cmd);
  }
  foreach (SI_NetLabel* netlabel, query.getNetLabels()) {
    mCenterPos += netlabel->getPosition();
    ++mItemCount;
    CmdSchematicNetLabelEdit* cmd = new CmdSchematicNetLabelEdit(*netlabel);
    mNetLabelEditCmds.append(cmd);
  }
  foreach (SI_Polygon* polygon, query.getPolygons()) {
    for (const Vertex& vertex : polygon->getPolygon().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      ++mItemCount;
    }
    CmdPolygonEdit* cmd = new CmdPolygonEdit(polygon->getPolygon());
    mPolygonEditCmds.append(cmd);
  }
  foreach (SI_Text* text, query.getTexts()) {
    // do not count texts of symbols if the symbol is selected too
    if ((!text->getSymbol()) ||
        (!query.getSymbols().contains(text->getSymbol()))) {
      mCenterPos += text->getPosition();
      ++mItemCount;
    }
    CmdTextEdit* cmd = new CmdTextEdit(text->getTextObj());
    mTextEditCmds.append(cmd);
  }
  foreach (SI_Image* image, query.getImages()) {
    // As the image does not support mirroring, its origin will move when
    // mirror is invoked. TO avoid drifting away, we its the center point here.
    mCenterPos += image->getImage()->getCenter();
    ++mItemCount;
    CmdImageEdit* cmd = new CmdImageEdit(*image->getImage());
    mImageEditCmds.append(cmd);
  }

  // Note: If only 1 item is selected, use its exact position as center.
  if (mItemCount > 1) {
    mCenterPos /= mItemCount;
    mCenterPos.mapToGrid(mSchematic.getGridInterval());
  }
}

CmdDragSelectedSchematicItems::~CmdDragSelectedSchematicItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedSchematicItems::snapToGrid() noexcept {
  const PositiveLength grid = mSchematic.getGridInterval();
  foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdSchematicBusJunctionEdit* cmd, mBusJunctionEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdSchematicBusLabelEdit* cmd, mBusLabelEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdImageEdit* cmd, mImageEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  mSnappedToGrid = true;
}

void CmdDragSelectedSchematicItems::resetAllTexts() noexcept {
  mTextsReset = true;
}

void CmdDragSelectedSchematicItems::setCurrentPosition(
    const Point& pos) noexcept {
  Point delta = pos - mStartPos;
  delta.mapToGrid(mSchematic.getGridInterval());

  if (delta != mDeltaPos) {
    // move selected elements
    foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdSchematicBusJunctionEdit* cmd, mBusJunctionEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    foreach (CmdSchematicBusLabelEdit* cmd, mBusLabelEditCmds) {
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
    foreach (CmdImageEdit* cmd, mImageEditCmds) {
      cmd->translate(delta - mDeltaPos, true);
    }
    mDeltaPos = delta;
  }
}

void CmdDragSelectedSchematicItems::rotate(
    const Angle& angle, bool aroundCurrentPosition) noexcept {
  const Point center = (aroundCurrentPosition && (mItemCount > 1))
      ? (mStartPos + mDeltaPos).mappedToGrid(mSchematic.getGridInterval())
      : (mCenterPos + mDeltaPos);

  // rotate selected elements
  foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdSchematicBusJunctionEdit* cmd, mBusJunctionEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdSchematicBusLabelEdit* cmd, mBusLabelEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(angle, center, true);
  }
  foreach (CmdImageEdit* cmd, mImageEditCmds) {
    cmd->rotate(angle, center, true);
  }
  mDeltaAngle += angle;
}

void CmdDragSelectedSchematicItems::mirror(
    Qt::Orientation orientation, bool aroundCurrentPosition) noexcept {
  const Point center = (aroundCurrentPosition && (mItemCount > 1))
      ? (mStartPos + mDeltaPos).mappedToGrid(mSchematic.getGridInterval())
      : (mCenterPos + mDeltaPos);

  // rotate selected elements
  foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds) {
    cmd->mirror(center, orientation, true);
  }
  foreach (CmdSchematicBusJunctionEdit* cmd, mBusJunctionEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  foreach (CmdSchematicBusLabelEdit* cmd, mBusLabelEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->mirrorGeometry(orientation, center, true);
  }
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  foreach (CmdImageEdit* cmd, mImageEditCmds) {
    cmd->mirror(orientation, center, true);
  }
  mMirrored = !mMirrored;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedSchematicItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaAngle == Angle::deg0()) &&
      (!mSnappedToGrid) && (!mMirrored) && (!mTextsReset)) {
    // no movement required --> discard all move commands
    deleteAllCommands();
    return false;
  }

  if (!mTextsReset) {
    qDeleteAll(mSymbolTextsResetCmds);
    mSymbolTextsResetCmds.clear();
  }

  // The original foreach loops are replaced with while loops here as an
  // additional safety measure against throw conditions.
  while (!mSymbolEditCmds.isEmpty()) {
    appendChild(mSymbolEditCmds.takeLast());  // can throw
  }
  while (!mSymbolTextsResetCmds.isEmpty()) {
    appendChild(mSymbolTextsResetCmds.takeLast());  // can throw
  }
  while (!mBusJunctionEditCmds.isEmpty()) {
    appendChild(mBusJunctionEditCmds.takeLast());  // can throw
  }
  while (!mBusLabelEditCmds.isEmpty()) {
    appendChild(mBusLabelEditCmds.takeLast());  // can throw
  }
  while (!mNetPointEditCmds.isEmpty()) {
    appendChild(mNetPointEditCmds.takeLast());  // can throw
  }
  while (!mNetLabelEditCmds.isEmpty()) {
    appendChild(mNetLabelEditCmds.takeLast());  // can throw
  }
  while (!mPolygonEditCmds.isEmpty()) {
    appendChild(mPolygonEditCmds.takeLast());  // can throw
  }
  while (!mTextEditCmds.isEmpty()) {
    appendChild(mTextEditCmds.takeLast());  // can throw
  }
  while (!mImageEditCmds.isEmpty()) {
    appendChild(mImageEditCmds.takeLast());  // can throw
  }
  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

void CmdDragSelectedSchematicItems::performPostExecution() noexcept {
  mSchematic.updateAllLabelAnchors();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdDragSelectedSchematicItems::deleteAllCommands() noexcept {
  qDeleteAll(mSymbolEditCmds);
  mSymbolEditCmds.clear();
  qDeleteAll(mSymbolTextsResetCmds);
  mSymbolTextsResetCmds.clear();
  qDeleteAll(mBusJunctionEditCmds);
  mBusJunctionEditCmds.clear();
  qDeleteAll(mBusLabelEditCmds);
  mBusLabelEditCmds.clear();
  qDeleteAll(mNetPointEditCmds);
  mNetPointEditCmds.clear();
  qDeleteAll(mNetLabelEditCmds);
  mNetLabelEditCmds.clear();
  qDeleteAll(mPolygonEditCmds);
  mPolygonEditCmds.clear();
  qDeleteAll(mTextEditCmds);
  mTextEditCmds.clear();
  qDeleteAll(mImageEditCmds);
  mImageEditCmds.clear();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
