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
#include "cmdpasteboarditems.h"

#include "../../graphics/polygongraphicsitem.h"
#include "../../project/cmd/cmdboardholeadd.h"
#include "../../project/cmd/cmdboardnetsegmentadd.h"
#include "../../project/cmd/cmdboardnetsegmentaddelements.h"
#include "../../project/cmd/cmdboardplaneadd.h"
#include "../../project/cmd/cmdboardpolygonadd.h"
#include "../../project/cmd/cmdboardstroketextadd.h"
#include "../../project/cmd/cmddeviceinstanceadd.h"
#include "../../project/cmd/cmdnetclassadd.h"
#include "../../project/cmd/cmdnetsignaladd.h"
#include "../../project/cmd/cmdprojectlibraryaddelement.h"
#include "../boardeditor/boardclipboarddata.h"
#include "../boardeditor/boardgraphicsscene.h"
#include "../boardeditor/boardnetsegmentsplitter.h"
#include "../boardeditor/graphicsitems/bgi_device.h"
#include "../boardeditor/graphicsitems/bgi_hole.h"
#include "../boardeditor/graphicsitems/bgi_netline.h"
#include "../boardeditor/graphicsitems/bgi_netpoint.h"
#include "../boardeditor/graphicsitems/bgi_plane.h"
#include "../boardeditor/graphicsitems/bgi_stroketext.h"
#include "../boardeditor/graphicsitems/bgi_via.h"
#include "cmdremoveboarditems.h"

#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPasteBoardItems::CmdPasteBoardItems(BoardGraphicsScene& scene,
                                       std::unique_ptr<BoardClipboardData> data,
                                       const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Board Elements")),
    mScene(scene),
    mBoard(mScene.getBoard()),
    mProject(mBoard.getProject()),
    mData(std::move(data)),
    mPosOffset(posOffset) {
  Q_ASSERT(mData);
}

CmdPasteBoardItems::~CmdPasteBoardItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPasteBoardItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Notes:
  //
  //  - Devices are only pasted if the corresponding component exists in the
  //    circuit, and the device does not yet exist on the board (one cannot
  //    paste a device if it is already added to the board).
  //  - Netlines which were attached to a pad or via which was not copy/pasted
  //    will be attached to newly created freestanding netpoints.
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  // Paste devices which do not yet exist on the board
  QSet<Uuid> pastedDevices;
  for (const BoardClipboardData::Device& dev : mData->getDevices()) {
    ComponentInstance* cmpInst =
        mProject.getCircuit().getComponentInstanceByUuid(dev.componentUuid);
    if (!cmpInst) {
      continue;  // Corresponding component does not exist (anymore) in circuit.
    }
    BI_Device* devInst =
        mBoard.getDeviceInstanceByComponentUuid(dev.componentUuid);
    if (devInst) {
      continue;  // Device already exist on the board.
    }

    // Copy new device to project library, if not existing already
    tl::optional<Uuid> pgkUuid;
    if (const Device* libDev =
            mProject.getLibrary().getDevice(dev.libDeviceUuid)) {
      pgkUuid = libDev->getPackageUuid();
    } else {
      std::unique_ptr<Device> newLibDev =
          Device::open(mData->getDirectory("dev/" % dev.libDeviceUuid.toStr()));
      pgkUuid = newLibDev->getPackageUuid();
      execNewChildCmd(new CmdProjectLibraryAddElement<Device>(
          mProject.getLibrary(), *newLibDev.release()));
    }
    Q_ASSERT(pgkUuid);

    // Copy new package to project library, if not existing already
    if (!mProject.getLibrary().getPackage(*pgkUuid)) {
      std::unique_ptr<Package> newLibPgk =
          Package::open(mData->getDirectory("pkg/" % pgkUuid->toStr()));
      execNewChildCmd(new CmdProjectLibraryAddElement<Package>(
          mProject.getLibrary(), *newLibPgk.release()));
    }

    // Add device instance to board
    QScopedPointer<BI_Device> device(new BI_Device(
        mBoard, *cmpInst, dev.libDeviceUuid, dev.libFootprintUuid,
        dev.position + mPosOffset, dev.rotation, dev.mirrored, false));
    for (const StrokeText& text : dev.strokeTexts) {
      // Note: Keep the UUID since it acts as a reference to the original
      // library footprint text.
      StrokeText copy(text);
      copy.setPosition(copy.getPosition() + mPosOffset);  // move
      BI_StrokeText* item = new BI_StrokeText(mBoard, copy);
      device->addStrokeText(*item);
    }
    execNewChildCmd(new CmdDeviceInstanceAdd(*device));
    if (auto item = mScene.getDevices().value(device.take())) {
      item->setSelected(true);
    }
    pastedDevices.insert(dev.componentUuid);
  }

  // Paste net segments
  for (const BoardClipboardData::NetSegment& seg : mData->getNetSegments()) {
    BoardNetSegmentSplitter splitter;
    for (auto it = mData->getPadPositions().constBegin();
         it != mData->getPadPositions().constEnd(); ++it) {
      const Uuid& device = it.key().first;
      const Uuid& pad = it.key().second;
      if (!pastedDevices.contains(device)) {
        // Device was not pasted, so we have to replace all pads by junctions
        splitter.replaceFootprintPadByJunctions(TraceAnchor::pad(device, pad),
                                                it.value());
      }
    }
    for (const Via& v : seg.vias) {
      splitter.addVia(v, false);
    }
    for (const Junction& junction : seg.junctions) {
      splitter.addJunction(junction);
    }
    for (const Trace& trace : seg.traces) {
      splitter.addTrace(trace);
    }

    foreach (const BoardNetSegmentSplitter::Segment& segment,
             splitter.split()) {
      // Add new segment
      NetSignal* netsignal =
          seg.netName ? getOrCreateNetSignal(**seg.netName) : nullptr;
      BI_NetSegment* copy =
          new BI_NetSegment(mBoard, Uuid::createRandom(), netsignal);
      execNewChildCmd(new CmdBoardNetSegmentAdd(*copy));

      // Add vias, netpoints and netlines
      QScopedPointer<CmdBoardNetSegmentAddElements> cmdAddElements(
          new CmdBoardNetSegmentAddElements(*copy));
      QHash<Uuid, BI_Via*> viaMap;
      for (const Via& v : segment.vias) {
        BI_Via* via = cmdAddElements->addVia(
            Via(Uuid::createRandom(), v.getPosition() + mPosOffset, v.getSize(),
                v.getDrillDiameter()));
        viaMap.insert(v.getUuid(), via);
      }
      QHash<Uuid, BI_NetPoint*> netPointMap;
      for (const Junction& junction : segment.junctions) {
        BI_NetPoint* netpoint =
            cmdAddElements->addNetPoint(junction.getPosition() + mPosOffset);
        netPointMap.insert(junction.getUuid(), netpoint);
      }
      for (const Trace& trace : segment.traces) {
        BI_NetLineAnchor* start = nullptr;
        if (tl::optional<Uuid> anchor =
                trace.getStartPoint().tryGetJunction()) {
          start = netPointMap[*anchor];
        } else if (tl::optional<Uuid> anchor =
                       trace.getStartPoint().tryGetVia()) {
          start = viaMap[*anchor];
        } else if (tl::optional<TraceAnchor::PadAnchor> anchor =
                       trace.getStartPoint().tryGetPad()) {
          Q_ASSERT(pastedDevices.contains(anchor->device));
          BI_Device* device =
              mBoard.getDeviceInstanceByComponentUuid(anchor->device);
          start = device ? device->getPad(anchor->pad) : nullptr;
        }
        BI_NetLineAnchor* end = nullptr;
        if (tl::optional<Uuid> anchor = trace.getEndPoint().tryGetJunction()) {
          end = netPointMap[*anchor];
        } else if (tl::optional<Uuid> anchor =
                       trace.getEndPoint().tryGetVia()) {
          end = viaMap[*anchor];
        } else if (tl::optional<TraceAnchor::PadAnchor> anchor =
                       trace.getEndPoint().tryGetPad()) {
          Q_ASSERT(pastedDevices.contains(anchor->device));
          BI_Device* device =
              mBoard.getDeviceInstanceByComponentUuid(anchor->device);
          end = device ? device->getPad(anchor->pad) : nullptr;
        }
        if ((!start) || (!end)) {
          throw LogicError(__FILE__, __LINE__);
        }
        cmdAddElements->addNetLine(*start, *end, trace.getLayer(),
                                   trace.getWidth());
      }
      execNewChildCmd(cmdAddElements.take());

      // Select pasted net segment items.
      foreach (BI_Via* via, copy->getVias()) {
        if (auto item = mScene.getVias().value(via)) {
          item->setSelected(true);
        }
      }
      foreach (BI_NetPoint* netPoint, copy->getNetPoints()) {
        if (auto item = mScene.getNetPoints().value(netPoint)) {
          item->setSelected(true);
        }
      }
      foreach (BI_NetLine* netLine, copy->getNetLines()) {
        if (auto item = mScene.getNetLines().value(netLine)) {
          item->setSelected(true);
        }
      }
    }
  }

  // Paste planes
  for (const BoardClipboardData::Plane& plane : mData->getPlanes()) {
    BI_Plane* copy =
        new BI_Plane(mBoard,
                     Uuid::createRandom(),  // assign new UUID
                     *plane.layer, *getOrCreateNetSignal(*plane.netSignalName),
                     plane.outline.translated(mPosOffset)  // move
        );
    copy->setMinWidth(plane.minWidth);
    copy->setMinClearance(plane.minClearance);
    copy->setKeepOrphans(plane.keepOrphans);
    copy->setPriority(plane.priority);
    copy->setConnectStyle(plane.connectStyle);
    execNewChildCmd(new CmdBoardPlaneAdd(*copy));
    if (auto item = mScene.getPlanes().value(copy)) {
      item->setSelected(true);
    }
  }

  // Paste polygons
  for (const Polygon& polygon : mData->getPolygons()) {
    Polygon copy(Uuid::createRandom(), polygon);  // assign new UUID
    copy.setPath(copy.getPath().translated(mPosOffset));  // move
    BI_Polygon* item = new BI_Polygon(mBoard, copy);
    execNewChildCmd(new CmdBoardPolygonAdd(*item));
    if (auto graphicsItem = mScene.getPolygons().value(item)) {
      graphicsItem->setSelected(true);
    }
  }

  // Paste stroke texts
  for (const StrokeText& text : mData->getStrokeTexts()) {
    StrokeText copy(Uuid::createRandom(), text);  // assign new UUID
    copy.setPosition(copy.getPosition() + mPosOffset);  // move
    BI_StrokeText* item = new BI_StrokeText(mBoard, copy);
    execNewChildCmd(new CmdBoardStrokeTextAdd(*item));
    if (auto graphicsItem = mScene.getStrokeTexts().value(item)) {
      graphicsItem->setSelected(true);
    }
  }

  // Paste holes
  for (const BoardHoleData& hole : mData->getHoles()) {
    BoardHoleData copy(Uuid::createRandom(), hole);  // assign new UUID
    copy.setPath(NonEmptyPath(copy.getPath()->translated(mPosOffset)));  // move
    BI_Hole* item = new BI_Hole(mBoard, copy);
    execNewChildCmd(new CmdBoardHoleAdd(*item));
    if (auto graphicsItem = mScene.getHoles().value(item)) {
      graphicsItem->setSelected(true);
    }
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

NetSignal* CmdPasteBoardItems::getOrCreateNetSignal(const QString& name) {
  NetSignal* netSignal = mProject.getCircuit().getNetSignalByName(name);
  if (netSignal) {
    return netSignal;
  }

  // Get or create netclass with the name "default"
  NetClass* netclass =
      mProject.getCircuit().getNetClassByName(ElementName("default"));
  if (!netclass) {
    CmdNetClassAdd* cmd =
        new CmdNetClassAdd(mProject.getCircuit(), ElementName("default"));
    execNewChildCmd(cmd);
    netclass = cmd->getNetClass();
    Q_ASSERT(netclass);
  }

  // Create new net signal
  CmdNetSignalAdd* cmdAddNetSignal =
      new CmdNetSignalAdd(mProject.getCircuit(), *netclass);
  execNewChildCmd(cmdAddNetSignal);
  return cmdAddNetSignal->getNetSignal();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
