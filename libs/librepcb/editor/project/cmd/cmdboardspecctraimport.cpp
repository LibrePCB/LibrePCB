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
#include "cmdboardspecctraimport.h"

#include "cmdboardnetsegmentadd.h"
#include "cmdboardnetsegmentaddelements.h"
#include "cmdboardnetsegmentremove.h"
#include "cmddeviceinstanceeditall.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/boardnetsegmentsplitter.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static CmdBoardSpecctraImport::Side parseSide(const SExpression& node) {
  if (node.getValue() == "front") {
    return CmdBoardSpecctraImport::Side::Front;
  } else if (node.getValue() == "back") {
    return CmdBoardSpecctraImport::Side::Back;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid board side: %1").arg(node.getValue()));
  }
}

static Length parseLength(const SExpression& node) {
  bool ok;
  qlonglong value = node.getValue().toLongLong(&ok);
  if (!ok)
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid number: %1").arg(node.getValue()));
  return Length(value);
}

CmdBoardSpecctraImport::CmdBoardSpecctraImport(
    Board& board, const SExpression& root,
    std::shared_ptr<MessageLogger> logger)
  : UndoCommandGroup(tr("Import From Specctra Session")),
    mProject(board.getProject()),
    mCircuit(mProject.getCircuit()),
    mBoard(board),
    mLogger(logger) {
  // Check file type.
  if (root.getName() != "session") {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("The specified file is not a Specctra session (SES)."));
  }

  // Check parser.
  const QString hostCad = root.getChild("routes/parser/host_cad/@0").getValue();
  const QString hostVersion =
      root.getChild("routes/parser/host_version/@0").getValue();
  if ((hostCad != qApp->applicationName()) ||
      (hostVersion != qApp->applicationVersion())) {
    mLogger->warning(
        tr("Specctra session originates from %1, compatibility is unknown.")
            .arg(hostCad + " " + hostVersion));
  }

  // Check resolution.
  if (root.getChild("placement/resolution/@0").getValue() != "mm") {
    throw RuntimeError(__FILE__, __LINE__,
                       "Unexpected unit, only 'mm' is supported.");
  }
  if (root.getChild("placement/resolution/@1").getValue() != "1000000") {
    throw RuntimeError(__FILE__, __LINE__,
                       "Unexpected resolution, only '1000000' is supported.");
  }

  // Parse components.
  for (const SExpression* cmpNode :
       root.getChild("placement").getChildren("component")) {
    QList<const SExpression*> childs = cmpNode->getChildren("place");
    if (childs.count() != 1) {
      throw RuntimeError(__FILE__, __LINE__,
                         "Unexpected component placement count.");
    }
    const SExpression* node = childs.first();
    QString name = node->getChild(0).getValue();
    Point pos(parseLength(node->getChild(1)), parseLength(node->getChild(2)));
    Side side = parseSide(node->getChild(3));
    Angle rot = deserialize<Angle>(node->getChild(4));
    mComponents.append(ComponentOut{name, pos, side, rot});
  }

  // Parse pad stacks.
  for (const SExpression* padStackNode :
       root.getChild("routes/library_out").getChildren("padstack")) {
    QString name = padStackNode->getChild(0).getValue();
    if (mPadStacks.contains(name)) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Pad stack '%1' defined multiple times.").arg(name));
    }
    QSet<Length> diameters;
    QList<const Layer*> layers;
    for (const SExpression* shapeNode :
         padStackNode->getChildren(SExpression::Type::List)) {
      for (const SExpression* childNode :
           shapeNode->getChildren(SExpression::Type::List)) {
        if (childNode->getName() != "circle") {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Unsupported pad stack shape '%1'.")
                                 .arg(shapeNode->getName()));
        }
        layers.append(deserialize<const Layer*>(childNode->getChild(0)));
        diameters.insert(parseLength(childNode->getChild(1)));
      }
    }
    if (diameters.count() != 1) {
      throw RuntimeError(__FILE__, __LINE__, "Unsupported complex pad stack.");
    }
    if (layers.count() < 2) {
      throw RuntimeError(__FILE__, __LINE__, "Too few layers in pad stack.");
    }
    std::sort(layers.begin(), layers.end(), [](const Layer* a, const Layer* b) {
      return a->getCopperNumber() < b->getCopperNumber();
    });
    for (const Layer* layer : mBoard.getCopperLayers()) {
      if ((layer->getCopperNumber() > layers.first()->getCopperNumber()) &&
          (layer->getCopperNumber() < layers.last()->getCopperNumber()) &&
          (!layers.contains(layer))) {
        throw RuntimeError(__FILE__, __LINE__, "Missing layers in pad stack.");
      }
    }
    mPadStacks[name] =
        PadStackOut{layers.first(), layers.last(), *diameters.begin()};
  }

  // Parse networks.
  for (const SExpression* netNode :
       root.getChild("routes/network_out").getChildren("net")) {
    NetOut net{netNode->getChild(0).getValue(), {}, {}};
    for (const SExpression* viaNode : netNode->getChildren("via")) {
      QString padStackId = viaNode->getChild(0).getValue();
      if (!mPadStacks.contains(padStackId)) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Pad stack '%1' not found.").arg(padStackId));
      }
      Point pos(parseLength(viaNode->getChild(1)),
                parseLength(viaNode->getChild(2)));
      net.vias.append(ViaOut{padStackId, pos});
    }
    for (const SExpression* wireNode : netNode->getChildren("wire")) {
      for (const SExpression* pathNode : wireNode->getChildren("path")) {
        QList<const SExpression*> childs =
            pathNode->getChildren(SExpression::Type::Token);
        if ((childs.count() < 2) || ((childs.count() % 2) != 0)) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Unexpected number of vertices in path element.");
        }
        WireOut wire{deserialize<const Layer*>(*childs.at(0)),
                     parseLength(*childs.at(1)),
                     {}};
        for (int i = 3; i < childs.count(); i += 2) {
          Length x = parseLength(*childs.at(i - 1));
          Length y = parseLength(*childs.at(i));
          wire.path.addVertex(Point(x, y));
        }
        if (wire.path.getVertices().count() < 2) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Path contains too few vertices.");
        }
        net.wires.append(wire);
      }
    }
    mNets.append(net);
  }
}

CmdBoardSpecctraImport::~CmdBoardSpecctraImport() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardSpecctraImport::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Delete all net segments. Not nice, but detecting unchanged objects is way
  // more complicated...
  foreach (auto seg, mBoard.getNetSegments()) {
    execNewChildCmd(new CmdBoardNetSegmentRemove(*seg));  // can throw
  }

  // Update devices placement.
  QSet<QString> components;
  for (const auto& item : mComponents) {
    components.insert(item.name);
    ComponentInstance* cmp = mCircuit.getComponentInstanceByName(item.name);
    BI_Device* dev =
        cmp ? mBoard.getDeviceInstanceByComponentUuid(cmp->getUuid()) : nullptr;
    if ((!cmp) || (!dev)) {
      mLogger->warning(tr("Component '%1' from Specctra session does not exist "
                          "in this board.")
                           .arg(item.name));
      continue;
    }
    if (((item.side == Side::Front) && (dev->getMirrored())) ||
        ((item.side == Side::Back) && (!dev->getMirrored()))) {
      mLogger->warning(
          tr("Component '%1' has been flipped, which is not supported yet.")
              .arg(item.name));
      continue;
    }
    if ((item.pos == dev->getPosition()) && (item.rot == dev->getRotation())) {
      continue;  // Nothing changed.
    }
    QScopedPointer<CmdDeviceInstanceEditAll> cmd(
        new CmdDeviceInstanceEditAll(*dev));
    cmd->setPosition(item.pos, false);
    cmd->setRotation(item.rot, false);
    execNewChildCmd(cmd.take());
  }

  // Warn about missing components.
  for (const auto dev : mBoard.getDeviceInstances()) {
    const QString name = *dev->getComponentInstance().getName();
    if (!components.contains(name)) {
      mLogger->warning(
          tr("The component '%1' does not exist in the Specctra session.")
              .arg(name));
    }
  }

  // Import nets.
  for (const auto& net : mNets) {
    NetSignal* netSignal = mCircuit.getNetSignalByName(net.netName);
    if (!netSignal) {
      mLogger->warning(tr("The net '%1' from Specctra session does not exist "
                          "in this project, skipping it.")
                           .arg(net.netName));
      continue;
    }

    // Helper data to memorize anchors.
    struct AnchorData {
      Point pos;
      const Layer* startLayer;
      const Layer* endLayer;
      TraceAnchor anchor;
    };
    QList<AnchorData> anchors;
    auto findAnchor = [&anchors](const Point& pos, const Layer* layer) {
      const int layerNumber = layer->getCopperNumber();
      for (auto& anchor : anchors) {
        if ((anchor.pos == pos) &&
            (layerNumber >= anchor.startLayer->getCopperNumber()) &&
            (layerNumber <= anchor.endLayer->getCopperNumber())) {
          return tl::optional(anchor.anchor);
        }
      }
      return tl::optional<TraceAnchor>();
    };

    // Add anchors for each pad corresponding to imported wire coordinates.
    QList<Point> wireCoordinates;
    QHash<const Layer*, QList<Point>> wireCoordinatesPerLayer;
    for (const auto& wire : net.wires) {
      for (const auto& vertex : wire.path.getVertices()) {
        wireCoordinates.append(vertex.getPos());
        wireCoordinatesPerLayer[wire.layer].append(vertex.getPos());
      }
    }
    for (const ComponentSignalInstance* cmpSig :
         netSignal->getComponentSignals()) {
      for (const BI_FootprintPad* pad : cmpSig->getRegisteredFootprintPads()) {
        Point pos = pad->getPosition();
        QList<Point>& coordinates = pad->getLibPad().isTht()
            ? wireCoordinates
            : wireCoordinatesPerLayer[&pad->getSmtLayer()];
        if (!coordinates.contains(pos)) {
          // Find another coordinate which is very close (rounding errors).
          std::sort(coordinates.begin(), coordinates.end(),
                    [&pos](const Point& a, const Point& b) {
                      return (a - pos).getLength() < (b - pos).getLength();
                    });
          if ((!coordinates.isEmpty()) &&
              ((coordinates.first() - pos).getLength() < Length(50))) {
            pos = coordinates.first();
          } else {
            continue;
          }
        }
        const Layer* startLayer = pad->getLibPad().isTht()
            ? &Layer::topCopper()
            : &pad->getSmtLayer();
        const Layer* endLayer = pad->getLibPad().isTht() ? &Layer::botCopper()
                                                         : &pad->getSmtLayer();
        anchors.append(AnchorData{
            pos, startLayer, endLayer,
            TraceAnchor::pad(pad->getDevice().getComponentInstanceUuid(),
                             pad->getLibPadUuid())});
      }
    }

    // Define net segments with BoardNetSegmentSplitter.
    BoardNetSegmentSplitter splitter;
    for (const auto& via : net.vias) {
      const PadStackOut& padStack = mPadStacks.value(via.padStackId);
      // Note: How can we know the drill diameter??? Use min. annular for now.
      const UnsignedLength annularWidth =
          mBoard.getDesignRules().getViaAnnularRing().getMinValue();
      const Uuid uuid = Uuid::createRandom();
      splitter.addVia(Via(uuid, *padStack.startLayer, *padStack.endLayer,
                          via.pos, PositiveLength(padStack.diameter),
                          PositiveLength(padStack.diameter - annularWidth * 2),
                          MaskConfig::automatic()),
                      false);
      anchors.append(AnchorData{via.pos, padStack.startLayer, padStack.endLayer,
                                TraceAnchor::via(uuid)});
    }
    auto getOrCreateAnchor = [&](const Point& pos, const Layer* layer) {
      if (auto anchor = findAnchor(pos, layer)) {
        return *anchor;
      } else {
        // Create new junction.
        const Uuid uuid = Uuid::createRandom();
        splitter.addJunction(Junction(uuid, pos));
        const TraceAnchor a = TraceAnchor::junction(uuid);
        anchors.append(AnchorData{pos, layer, layer, a});
        return a;
      }
    };
    for (const auto& wire : net.wires) {
      for (int i = 1; i < wire.path.getVertices().count(); ++i) {
        const Point p0 = wire.path.getVertices().at(i - 1).getPos();
        const Point p1 = wire.path.getVertices().at(i).getPos();
        splitter.addTrace(Trace(Uuid::createRandom(), *wire.layer,
                                PositiveLength(wire.width),
                                getOrCreateAnchor(p0, wire.layer),
                                getOrCreateAnchor(p1, wire.layer)));
      }
    }

    // Add netpoints, vias and traces for each segment.
    foreach (const BoardNetSegmentSplitter::Segment& segment,
             splitter.split()) {
      // Add new segment
      BI_NetSegment* copy =
          new BI_NetSegment(mBoard, Uuid::createRandom(), netSignal);
      execNewChildCmd(new CmdBoardNetSegmentAdd(*copy));

      // Add vias, netpoints and netlines
      QScopedPointer<CmdBoardNetSegmentAddElements> cmdAddElements(
          new CmdBoardNetSegmentAddElements(*copy));
      QHash<Uuid, BI_Via*> viaMap;
      for (const Via& v : segment.vias) {
        BI_Via* via = cmdAddElements->addVia(
            Via(Uuid::createRandom(), v.getStartLayer(), v.getEndLayer(),
                v.getPosition(), v.getSize(), v.getDrillDiameter(),
                v.getExposureConfig()));
        viaMap.insert(v.getUuid(), via);
      }
      QHash<Uuid, BI_NetPoint*> netPointMap;
      for (const Junction& junction : segment.junctions) {
        BI_NetPoint* netpoint =
            cmdAddElements->addNetPoint(junction.getPosition());
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
    }
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
