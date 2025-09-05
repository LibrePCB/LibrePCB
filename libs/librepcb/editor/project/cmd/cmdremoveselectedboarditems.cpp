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
#include "cmdremoveselectedboarditems.h"

#include "../board/boardgraphicsscene.h"
#include "../board/boardselectionquery.h"
#include "cmdremoveboarditems.h"

#include <librepcb/core/project/board/board.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdRemoveSelectedBoardItems::CmdRemoveSelectedBoardItems(
    BoardGraphicsScene& scene, bool includeLockedItems) noexcept
  : UndoCommand(tr("Remove Board Items")),
    mScene(scene),
    mIncludeLockedItems(includeLockedItems) {
}

CmdRemoveSelectedBoardItems::~CmdRemoveSelectedBoardItems() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

QList<BI_NetSegment*> CmdRemoveSelectedBoardItems::getModifiedNetSegments()
    const noexcept {
  return mWrappedCommand ? mWrappedCommand->getModifiedNetSegments()
                         : QList<BI_NetSegment*>{};
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveSelectedBoardItems::performExecute() {
  // get all selected items
  BoardSelectionQuery query(mScene, mIncludeLockedItems);
  query.addDeviceInstancesOfSelectedFootprints();
  query.addSelectedBoardPads();
  query.addSelectedVias();
  query.addSelectedNetLines();
  query.addNetPointsOfNetLines(true);
  query.addSelectedPlanes();
  query.addSelectedZones();
  query.addSelectedPolygons();
  query.addSelectedBoardStrokeTexts();
  query.addSelectedFootprintStrokeTexts();
  query.addSelectedHoles();

  // clear selection because these items will be removed now
  mScene.clearSelection();

  // remove items
  mWrappedCommand.reset(new CmdRemoveBoardItems(mScene.getBoard()));
  mWrappedCommand->removeDeviceInstances(query.getDeviceInstances());
  mWrappedCommand->removeBoardPads(query.getPads());
  mWrappedCommand->removeVias(query.getVias());
  mWrappedCommand->removeNetLines(query.getNetLines());
  mWrappedCommand->removePlanes(query.getPlanes());
  mWrappedCommand->removeZones(query.getZones());
  mWrappedCommand->removePolygons(query.getPolygons());
  mWrappedCommand->removeStrokeTexts(query.getStrokeTexts());
  mWrappedCommand->removeHoles(query.getHoles());
  return mWrappedCommand->execute();
}

void CmdRemoveSelectedBoardItems::performUndo() {
  mWrappedCommand->undo();
}

void CmdRemoveSelectedBoardItems::performRedo() {
  mWrappedCommand->redo();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
