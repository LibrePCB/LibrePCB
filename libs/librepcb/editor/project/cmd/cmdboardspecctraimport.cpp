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

// Allow some procentual deviation due to floating point inaccuracy:
// 25nm <= 0.001% <= 1um
static bool fuzzyCompare(const Point& exact, const Point& imported) {
  const Length maxDim = std::max(imported.getX(), imported.getY());
  const Length epsilon = qBound(Length(25), maxDim / 100000, Length(1000));
  return (exact - imported).getLength() < epsilon;
}

static bool fuzzyCompare(const Angle& a, const Angle& b) {
  return (a - b).mappedTo180deg().abs() < Angle(100);  // 100 micro degrees.
}

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

static Length parseLength(const SExpression& node, double resolution) {
  bool ok;
  double value = node.getValue().toDouble(&ok);
  if (!ok)
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid number: %1").arg(node.getValue()));
  return Length::fromMm(value / resolution);
}

static Angle parseAngle(const SExpression& node) {
  Angle angle = Angle::fromDeg(deserialize<double>(node));
  const Angle multiple = Angle::fromDeg("1");
  if (fuzzyCompare(Angle::deg0(), angle % multiple)) {
    angle.round(multiple);
  }
  return angle;
}

static double getResolution(const SExpression& node, QString& logStr) {
  const SExpression& resolutionNode = node.getChild("resolution/@1");
  double resolution = deserialize<double>(resolutionNode);
  const QString unit = node.getChild("resolution/@0").getValue();
  if (unit == "um") {
    resolution /= 1000;
  } else if (unit != "mm") {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unsupported unit: '%1'").arg(unit));
  }
  logStr = QString("1/%1 %2").arg(resolutionNode.getValue()).arg(unit);
  return resolution;
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
  QString hostCad;
  if (const auto child = root.tryGetChild("routes/parser/host_cad/@0")) {
    hostCad = child->getValue();
  }
  QString hostVersion;
  if (const auto child = root.tryGetChild("routes/parser/host_version/@0")) {
    hostVersion = child->getValue();
  }
  if (hostCad.isEmpty()) {
    mLogger->warning(
        "Specctra session doesn't specify host CAD, compatibility is unknown.");
  } else if ((hostCad != qApp->applicationName()) ||
             (hostVersion != qApp->applicationVersion())) {
    mLogger->warning(
        QString(
            "Specctra session originates from %1, compatibility is unknown.")
            .arg(hostCad + " " + hostVersion));
  }

  // Parse placement.
  if (const SExpression* child = root.tryGetChild("placement")) {
    QString logRes;
    const double resolution = getResolution(*child, logRes);
    mLogger->debug("Placement resolution: " % logRes);
    mComponents = QList<ComponentOut>();
    for (const SExpression* cmpNode : child->getChildren("component")) {
      QList<const SExpression*> childs = cmpNode->getChildren("place");
      if (childs.count() != 1) {
        throw RuntimeError(__FILE__, __LINE__,
                           "Unexpected component placement count.");
      }
      const SExpression* node = childs.first();
      QString name = node->getChild("@0").getValue();
      Point pos(parseLength(node->getChild("@1"), resolution),
                parseLength(node->getChild("@2"), resolution));
      Side side = parseSide(node->getChild("@3"));
      Angle rot = parseAngle(node->getChild("@4"));
      mComponents->append(ComponentOut{name, pos, side, rot});
    }
  } else {
    mLogger->warning(
        "Specctra session doesn't contain component placement data.");
  }

  const SExpression& routesNode = root.getChild("routes");
  QString logRes;
  const double resolution = getResolution(routesNode, logRes);
  mLogger->debug("Routing resolution: " % logRes);

  // Parse pad stacks.
  for (const SExpression* padStackNode :
       routesNode.getChild("library_out").getChildren("padstack")) {
    QString name = padStackNode->getChild("@0").getValue();
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
        layers.append(deserialize<const Layer*>(childNode->getChild("@0")));
        diameters.insert(parseLength(childNode->getChild("@1"), resolution));
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
       routesNode.getChild("network_out").getChildren("net")) {
    NetOut net{netNode->getChild("@0").getValue(), {}, {}};
    for (const SExpression* viaNode : netNode->getChildren("via")) {
      QString padStackId = viaNode->getChild("@0").getValue();
      if (!mPadStacks.contains(padStackId)) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Pad stack '%1' not found.").arg(padStackId));
      }
      Point pos(parseLength(viaNode->getChild("@1"), resolution),
                parseLength(viaNode->getChild("@2"), resolution));
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
                     parseLength(*childs.at(1), resolution),
                     {}};
        if (wire.width <= 0) {
          mLogger->warning("Skippted wire with zero width.");
          continue;
        }
        for (int i = 3; i < childs.count(); i += 2) {
          Length x = parseLength(*childs.at(i - 1), resolution);
          Length y = parseLength(*childs.at(i), resolution);
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

  mLogger->debug(tr("Specctra session file parsed successfully."));
}

CmdBoardSpecctraImport::~CmdBoardSpecctraImport() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardSpecctraImport::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Memorize current board to allow reusing their properties.
  struct OldJunction {
    Uuid uuid;
    Point pos;
    const Layer* layer;  // May be nullptr.
  };
  struct OldTrace {
    Uuid uuid;
    Point p1;
    Point p2;
    const Layer* layer;
    Length width;
  };

  struct OldSegment {
    Uuid uuid;
    QPointer<NetSignal> net;
    QList<OldJunction> junctions;
    QList<OldTrace> traces;
    QList<Via> vias;
  };
  QList<OldSegment> oldSegments;
  foreach (const BI_NetSegment* seg, mBoard.getNetSegments()) {
    OldSegment oldSeg{seg->getUuid(), seg->getNetSignal(), {}, {}, {}};
    foreach (const BI_NetPoint* np, seg->getNetPoints()) {
      oldSeg.junctions.append(OldJunction{np->getUuid(), np->getPosition(),
                                          np->getLayerOfTraces()});
    }
    foreach (const BI_NetLine* nl, seg->getNetLines()) {
      oldSeg.traces.append(OldTrace{nl->getUuid(), nl->getP1().getPosition(),
                                    nl->getP2().getPosition(), &nl->getLayer(),
                                    *nl->getWidth()});
    }
    foreach (const BI_Via* via, seg->getVias()) {
      oldSeg.vias.append(via->getVia());
    }
    oldSegments.append(oldSeg);
  }

  // Helper functions to find corresponding old objects for new objects.
  QSet<Uuid> reusedUuids;
  int newUuids = 0;
  auto findNetPoint = [&](const NetSignal* net, const Point& pos,
                          const Layer* layer) {
    for (const OldSegment& seg : oldSegments) {
      if (seg.net == net) {
        for (const OldJunction& np : seg.junctions) {
          if (fuzzyCompare(np.pos, pos) && (np.layer == layer) &&
              (!reusedUuids.contains(np.uuid))) {
            reusedUuids.insert(np.uuid);
            return std::make_optional(np);
          }
        }
      }
    }
    ++newUuids;
    return std::optional<OldJunction>();
  };
  auto findNetLineImpl = [&](const NetSignal* net, Point p1, Point p2,
                             const Layer& layer,
                             const std::optional<Length>& width) {
    if (p1 > p2) std::swap(p1, p2);
    for (const OldSegment& seg : oldSegments) {
      if (seg.net == net) {
        for (const OldTrace& nl : seg.traces) {
          Point nlP1 = nl.p1;
          Point nlP2 = nl.p2;
          if (nlP1 > nlP2) std::swap(nlP1, nlP2);
          if (fuzzyCompare(nlP1, p1) && fuzzyCompare(nlP2, p2) &&
              (nl.layer == &layer) && ((!width) || (nl.width == *width)) &&
              (!reusedUuids.contains(nl.uuid))) {
            reusedUuids.insert(nl.uuid);
            return std::make_optional(nl);
          }
        }
      }
    }
    return std::optional<OldTrace>();
  };
  auto findNetLine = [&](const NetSignal* net, Point p1, Point p2,
                         const Layer& layer, const Length& width) {
    // First try to match including trace width, then ignore it because it
    // might have been changed during the DSN -> SES roundtrip.
    if (auto nl = findNetLineImpl(net, p1, p2, layer, width)) {
      return nl;
    } else if (auto nl = findNetLineImpl(net, p1, p2, layer, std::nullopt)) {
      return nl;
    } else {
      ++newUuids;
      return std::optional<OldTrace>();
    }
  };
  auto findVia = [&](const NetSignal* net, const Point& pos, const Layer& start,
                     const Layer& end) {
    for (const OldSegment& seg : oldSegments) {
      if (seg.net == net) {
        for (const Via& via : seg.vias) {
          if (fuzzyCompare(via.getPosition(), pos) &&
              (via.getStartLayer() == start) && (via.getEndLayer() == end) &&
              (!reusedUuids.contains(via.getUuid()))) {
            reusedUuids.insert(via.getUuid());
            return std::make_optional(via);
          }
        }
      }
    }
    ++newUuids;
    return std::optional<Via>();
  };
  auto anyRefInSegment = [](const OldSegment& seg, const QSet<Uuid>& refs) {
    foreach (const OldJunction& np, seg.junctions) {
      if (refs.contains(np.uuid)) {
        return true;
      }
    }
    foreach (const OldTrace& nl, seg.traces) {
      if (refs.contains(nl.uuid)) {
        return true;
      }
    }
    foreach (const Via& via, seg.vias) {
      if (refs.contains(via.getUuid())) {
        return true;
      }
    }
    return false;
  };
  auto findNetSegment = [&](const NetSignal* net, const QSet<Uuid>& refs) {
    for (const OldSegment& seg : oldSegments) {
      if (seg.net == net) {
        if (anyRefInSegment(seg, refs) && (!reusedUuids.contains(seg.uuid))) {
          reusedUuids.insert(seg.uuid);
          return std::make_optional(seg);
        }
      }
    }
    ++newUuids;
    return std::optional<OldSegment>();
  };

  //////////////////////////////////////////////////////////////////////////////

  // Delete all net segments, they will be re-created from scratch below.
  foreach (auto seg, mBoard.getNetSegments()) {
    execNewChildCmd(new CmdBoardNetSegmentRemove(*seg));  // can throw
  }

  // Update devices placement.
  QSet<QString> importedComponents;
  QSet<Uuid> updatedComponents;
  if (mComponents) {
    for (const auto& item : *mComponents) {
      importedComponents.insert(item.name);
      ComponentInstance* cmp = mCircuit.getComponentInstanceByName(item.name);
      BI_Device* dev = cmp
          ? mBoard.getDeviceInstanceByComponentUuid(cmp->getUuid())
          : nullptr;
      if ((!cmp) || (!dev)) {
        mLogger->warning(
            tr("Component '%1' from Specctra session does not exist "
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
      std::unique_ptr<CmdDeviceInstanceEditAll> cmd(
          new CmdDeviceInstanceEditAll(*dev));
      if (!fuzzyCompare(dev->getPosition(), item.pos)) {
        cmd->setPosition(item.pos, false);
        updatedComponents.insert(cmp->getUuid());
      }
      if (!fuzzyCompare(dev->getRotation(), item.rot)) {
        cmd->setRotation(item.rot, false);
        updatedComponents.insert(cmp->getUuid());
      }
      execNewChildCmd(cmd.release());
    }

    // Warn about missing components.
    for (const auto dev : mBoard.getDeviceInstances()) {
      const QString name = *dev->getComponentInstance().getName();
      // Footprints without pads are discarded by Freerouting as they are not
      // relevant, thus ignore them.
      if ((!importedComponents.contains(name)) && (!dev->getPads().isEmpty())) {
        mLogger->warning(
            tr("The component '%1' does not exist in the Specctra session.")
                .arg(name));
      }
    }
  }

  // Import nets.
  for (const auto& net : mNets) {
    NetSignal* netSignal = mCircuit.getNetSignalByName(net.netName);
    // ATTENTION: The ~anonymous~ comes from our own Specctra export!
    if ((!netSignal) && (!net.netName.startsWith("~anonymous~"))) {
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
          return std::make_optional(anchor.anchor);
        }
      }
      return std::optional<TraceAnchor>();
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
         (netSignal ? netSignal->getComponentSignals()
                    : QList<ComponentSignalInstance*>{})) {
      for (const BI_FootprintPad* pad : cmpSig->getRegisteredFootprintPads()) {
        Point pos = pad->getPosition();
        QList<Point>& coordinates = pad->getLibPad().isTht()
            ? wireCoordinates
            : wireCoordinatesPerLayer[&pad->getSolderLayer()];
        if (!coordinates.contains(pos)) {
          // Find another coordinate which is very close (rounding errors).
          // In some tests, errors were up to 70 nm!
          std::sort(coordinates.begin(), coordinates.end(),
                    [&pos](const Point& a, const Point& b) {
                      return (a - pos).getLength() < (b - pos).getLength();
                    });
          if ((!coordinates.isEmpty()) &&
              fuzzyCompare(pos, coordinates.first())) {
            pos = coordinates.first();
          } else {
            continue;
          }
        }
        const Layer* startLayer = pad->getLibPad().isTht()
            ? &Layer::topCopper()
            : &pad->getSolderLayer();
        const Layer* endLayer = pad->getLibPad().isTht()
            ? &Layer::botCopper()
            : &pad->getSolderLayer();
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
      const std::optional<Via> oldVia =
          findVia(netSignal, via.pos, *padStack.startLayer, *padStack.endLayer);
      const Uuid uuid = oldVia ? oldVia->getUuid() : Uuid::createRandom();
      // Note: How can we know the drill diameter??? Use this logic for now:
      //  - If position & size not modified, keep original drill diameter too
      //  - Try to extract drill diameter from pad stack ID
      //  - If this didn't work, use minimum annular ring as fallback
      std::optional<PositiveLength> drillDiameter;
      if (oldVia) {
        drillDiameter = oldVia->getDrillDiameter();
      } else if (auto dia = extractViaDrillDiameter(via.padStackId)) {
        drillDiameter = *dia;
      } else {
        const UnsignedLength annularWidth =
            mBoard.getDesignRules().getViaAnnularRing().getMinValue();
        drillDiameter = PositiveLength(padStack.diameter - annularWidth * 2);
      }
      // For the exposure config, use a similar mechanism.
      MaskConfig exposureConfig = MaskConfig::automatic();
      if (oldVia) {
        exposureConfig = oldVia->getExposureConfig();
      } else if (auto cfg = extractViaExposureConfig(via.padStackId)) {
        exposureConfig = *cfg;
      }
      splitter.addVia(Via(uuid, *padStack.startLayer, *padStack.endLayer,
                          oldVia ? oldVia->getPosition() : via.pos,
                          PositiveLength(padStack.diameter), *drillDiameter,
                          exposureConfig),
                      false);
      anchors.append(AnchorData{via.pos, padStack.startLayer, padStack.endLayer,
                                TraceAnchor::via(uuid)});
    }
    auto getOrCreateAnchor = [&](const Point& pos, const Layer* layer) {
      if (auto anchor = findAnchor(pos, layer)) {
        return *anchor;
      } else {
        // Create new junction.
        const std::optional<OldJunction> oldNp =
            findNetPoint(netSignal, pos, layer);
        const Uuid uuid = oldNp ? oldNp->uuid : Uuid::createRandom();
        splitter.addJunction(Junction(uuid, oldNp ? oldNp->pos : pos));
        const TraceAnchor a = TraceAnchor::junction(uuid);
        anchors.append(AnchorData{pos, layer, layer, a});
        return a;
      }
    };
    for (const auto& wire : net.wires) {
      for (int i = 1; i < wire.path.getVertices().count(); ++i) {
        Point p0 = wire.path.getVertices().at(i - 1).getPos();
        Point p1 = wire.path.getVertices().at(i).getPos();
        const std::optional<OldTrace> oldNl =
            findNetLine(netSignal, p0, p1, *wire.layer, wire.width);
        if (oldNl && (!fuzzyCompare(oldNl->p1, p0))) {
          std::swap(p0, p1);  // Avoid change in file format.
        }
        splitter.addTrace(Trace(oldNl ? oldNl->uuid : Uuid::createRandom(),
                                *wire.layer, PositiveLength(wire.width),
                                getOrCreateAnchor(p0, wire.layer),
                                getOrCreateAnchor(p1, wire.layer)));
      }
    }

    // Add netpoints, vias and traces for each segment.
    foreach (const BoardNetSegmentSplitter::Segment& segment,
             splitter.split()) {
      // Find old segment.
      QSet<Uuid> nsRefs;
      for (const Junction& junction : segment.junctions) {
        nsRefs.insert(junction.getUuid());
      }
      for (const Trace& trace : segment.traces) {
        nsRefs.insert(trace.getUuid());
      }
      for (const Via& via : segment.vias) {
        nsRefs.insert(via.getUuid());
      }
      const std::optional<OldSegment> oldNs = findNetSegment(netSignal, nsRefs);

      // Add new segment
      BI_NetSegment* copy = new BI_NetSegment(
          mBoard, oldNs ? oldNs->uuid : Uuid::createRandom(), netSignal);
      execNewChildCmd(new CmdBoardNetSegmentAdd(*copy));

      // Add vias, netpoints and netlines
      std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAddElements(
          new CmdBoardNetSegmentAddElements(*copy));
      QHash<Uuid, BI_Via*> viaMap;
      for (const Via& v : segment.vias) {
        BI_Via* via = cmdAddElements->addVia(Via(
            v.getUuid(), v.getStartLayer(), v.getEndLayer(), v.getPosition(),
            v.getSize(), v.getDrillDiameter(), v.getExposureConfig()));
        viaMap.insert(v.getUuid(), via);
      }
      QHash<Uuid, BI_NetPoint*> netPointMap;
      for (const Junction& junction : segment.junctions) {
        BI_NetPoint* netpoint =
            new BI_NetPoint(*copy, junction.getUuid(), junction.getPosition());
        cmdAddElements->addNetPoint(*netpoint);
        netPointMap.insert(junction.getUuid(), netpoint);
      }
      for (const Trace& trace : segment.traces) {
        BI_NetLineAnchor* p1 = nullptr;
        if (std::optional<Uuid> anchor = trace.getP1().tryGetJunction()) {
          p1 = netPointMap[*anchor];
        } else if (std::optional<Uuid> anchor = trace.getP1().tryGetVia()) {
          p1 = viaMap[*anchor];
        } else if (std::optional<TraceAnchor::PadAnchor> anchor =
                       trace.getP1().tryGetPad()) {
          BI_Device* device =
              mBoard.getDeviceInstanceByComponentUuid(anchor->device);
          p1 = device ? device->getPad(anchor->pad) : nullptr;
        }
        BI_NetLineAnchor* p2 = nullptr;
        if (std::optional<Uuid> anchor = trace.getP2().tryGetJunction()) {
          p2 = netPointMap[*anchor];
        } else if (std::optional<Uuid> anchor = trace.getP2().tryGetVia()) {
          p2 = viaMap[*anchor];
        } else if (std::optional<TraceAnchor::PadAnchor> anchor =
                       trace.getP2().tryGetPad()) {
          BI_Device* device =
              mBoard.getDeviceInstanceByComponentUuid(anchor->device);
          p2 = device ? device->getPad(anchor->pad) : nullptr;
        }
        if ((!p1) || (!p2)) {
          throw LogicError(__FILE__, __LINE__);
        }
        BI_NetLine* netline =
            new BI_NetLine(*copy, trace.getUuid(), *p1, *p2, trace.getLayer(),
                           trace.getWidth());
        cmdAddElements->addNetLine(*netline);
      }
      execNewChildCmd(cmdAddElements.release());
    }
  }

  // Print some statistics.
  mLogger->info(
      tr("Updated %1 components (%2 unmodified components skipped).")
          .arg(updatedComponents.count())
          .arg(importedComponents.count() - updatedComponents.count()));
  mLogger->info(tr("Updated %1 net objects (%2 unmodified objects skipped).")
                    .arg(newUuids)
                    .arg(reusedUuids.count()));

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

std::optional<PositiveLength> CmdBoardSpecctraImport::extractViaDrillDiameter(
    const QString& padStackId) noexcept {
  // Note: Keep in sync with BoardSpecctraExport::getWiringPadStackId().
  const QStringList tokens = padStackId.split("-");
  if ((tokens.count() >= 4) && (tokens[0] == "via")) {
    try {
      return PositiveLength(Length::fromMm(tokens[2]));
    } catch (const Exception&) {
      // Not critical.
    }
  }
  return std::nullopt;
}

std::optional<MaskConfig> CmdBoardSpecctraImport::extractViaExposureConfig(
    const QString& padStackId) noexcept {
  // Note: Keep in sync with BoardSpecctraExport::getWiringPadStackId().
  const QStringList tokens = padStackId.split("-");
  if (((tokens.count() > 3) || (tokens.count() < 6)) && (tokens[0] == "via")) {
    if (tokens.count() == 4) {
      return MaskConfig::off();
    } else if (tokens[4] == "exposed") {
      return MaskConfig::automatic();
    } else if (tokens[4].startsWith("exposed:")) {
      try {
        const QString offset = QString(tokens[4]).remove("exposed:");
        return MaskConfig::manual(Length::fromMm(offset));
      } catch (const Exception&) {
        // Not critical.
      }
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
