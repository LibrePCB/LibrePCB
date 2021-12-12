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
#include "cmdreplacedevice.h"

#include "../../project/cmd/cmdboardnetsegmentaddelements.h"
#include "../../project/cmd/cmddeviceinstanceremove.h"
#include "cmdadddevicetoboard.h"
#include "cmdremoveboarditems.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprint.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdReplaceDevice::CmdReplaceDevice(
    Workspace& workspace, Board& board, BI_Device& device,
    const Uuid& newDeviceUuid,
    const tl::optional<Uuid>& newFootprintUuid) noexcept
  : UndoCommandGroup(tr("Change Device")),
    mWorkspace(workspace),
    mBoard(board),
    mDeviceInstance(device),
    mNewDeviceUuid(newDeviceUuid),
    mNewFootprintUuid(newFootprintUuid) {
}

CmdReplaceDevice::~CmdReplaceDevice() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdReplaceDevice::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // remove all connected netlines
  foreach (BI_FootprintPad* pad, mDeviceInstance.getFootprint().getPads()) {
    BI_NetSegment* netsegment = pad->getNetSegmentOfLines();
    if (netsegment) {
      QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
          new CmdBoardNetSegmentAddElements(*netsegment));
      QMap<GraphicsLayer*, BI_NetPoint*> newNetPoints = {};
      QSet<BI_NetLine*> connectedNetLines = pad->getNetLines();
      if (connectedNetLines.count() > 1) {
        foreach (BI_NetLine* netline, connectedNetLines) {
          auto it = newNetPoints.find(&netline->getLayer());
          if (it == newNetPoints.end()) {
            it = newNetPoints.insert(&netline->getLayer(),
                                     cmdAdd->addNetPoint(pad->getPosition()));
          }
          cmdAdd->addNetLine(**it, *netline->getOtherPoint(*pad),
                             netline->getLayer(), netline->getWidth());
        }
      }
      execNewChildCmd(cmdAdd.take());
      QScopedPointer<CmdRemoveBoardItems> cmdRemove(
          new CmdRemoveBoardItems(netsegment->getBoard()));
      cmdRemove->removeNetLines(pad->getNetLines());
      execNewChildCmd(cmdRemove.take());  // can throw
    }
  }

  // replace the device instance
  execNewChildCmd(new CmdDeviceInstanceRemove(mDeviceInstance));  // can throw
  CmdAddDeviceToBoard* cmd = new CmdAddDeviceToBoard(
      mWorkspace, mBoard, mDeviceInstance.getComponentInstance(),
      mNewDeviceUuid, mNewFootprintUuid, mDeviceInstance.getPosition(),
      mDeviceInstance.getRotation(), mDeviceInstance.getIsMirrored());
  execNewChildCmd(cmd);  // can throw
  BI_Device* newDevice = cmd->getDeviceInstance();
  Q_ASSERT(newDevice);

  // TODO: reconnect all netpoints/netlines

  undoScopeGuard.dismiss();  // no undo required
  return (getChildCount() > 0);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
