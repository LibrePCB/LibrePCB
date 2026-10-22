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
#include "boardtools.h"

#include "designsession.h"
#include "mcpserver.h"
#include "toolhelpers.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/job/archiveoutputjob.h>
#include <librepcb/core/job/bomoutputjob.h>
#include <librepcb/core/job/gerberexcellonoutputjob.h>
#include <librepcb/core/job/graphicsoutputjob.h>
#include <librepcb/core/job/lppzoutputjob.h>
#include <librepcb/core/job/pickplaceoutputjob.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/org/organization.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/project/board/drc/boarddesignrulechecksettings.h>
#include <librepcb/core/project/board/items/bi_airwire.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netclass.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/outputjobrunner.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/utils/transform.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/project/cmd/cmdadddevicetoboard.h>
#include <librepcb/editor/project/cmd/cmdboardedit.h>
#include <librepcb/editor/project/cmd/cmdboardholeadd.h>
#include <librepcb/editor/project/cmd/cmdboardholeedit.h>
#include <librepcb/editor/project/cmd/cmdboardnetlineedit.h>
#include <librepcb/editor/project/cmd/cmdboardnetpointedit.h>
#include <librepcb/editor/project/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/editor/project/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/editor/project/cmd/cmdboardplaneadd.h>
#include <librepcb/editor/project/cmd/cmdboardplaneedit.h>
#include <librepcb/editor/project/cmd/cmdboardpolygonadd.h>
#include <librepcb/editor/project/cmd/cmdboardpolygonedit.h>
#include <librepcb/editor/project/cmd/cmdboardsplitnetline.h>
#include <librepcb/editor/project/cmd/cmdboardstroketextadd.h>
#include <librepcb/editor/project/cmd/cmdboardstroketextedit.h>
#include <librepcb/editor/project/cmd/cmdboardviaedit.h>
#include <librepcb/editor/project/cmd/cmdboardzoneadd.h>
#include <librepcb/editor/project/cmd/cmdboardzoneedit.h>
#include <librepcb/editor/project/cmd/cmdcombineboardnetsegments.h>
#include <librepcb/editor/project/cmd/cmddeviceinstanceeditall.h>
#include <librepcb/editor/project/cmd/cmddevicestroketextadd.h>
#include <librepcb/editor/project/cmd/cmddevicestroketextremove.h>
#include <librepcb/editor/project/cmd/cmdprojectedit.h>
#include <librepcb/editor/project/cmd/cmdremoveboarditems.h>
#include <librepcb/editor/project/cmd/cmdreplacedevice.h>
#include <librepcb/editor/project/cmd/cmdsimplifyboardnetsegments.h>
#include <librepcb/editor/undostack.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

using editor::UndoStackTransaction;

/*******************************************************************************
 *  Helpers
 ******************************************************************************/

static const Layer& copperLayer(const QJsonValue& value, const Board& board) {
  const QString name = value.toString("top");
  const int innerCount = board.getInnerLayerCount();
  if (name == "top") {
    return Layer::topCopper();
  } else if (name == "bottom") {
    return Layer::botCopper();
  } else if (name.startsWith("inner")) {
    bool ok = false;
    const int number = name.mid(5).toInt(&ok);
    if (ok && (number >= 1) && (number <= innerCount)) {
      return *Layer::innerCopper(number);
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      (innerCount > 0)
          ? QString("The layer must be 'top', 'bottom' or 'inner1' to "
                    "'inner%1'.")
                .arg(innerCount)
          : QString("The layer must be 'top' or 'bottom' (the board has no "
                    "inner layers)."));
}

static QString layerName(const Layer& layer) noexcept {
  if (layer == Layer::topCopper()) {
    return "top";
  } else if (layer == Layer::botCopper()) {
    return "bottom";
  } else if (layer.isInner()) {
    return QString("inner%1").arg(layer.getCopperNumber());
  }
  return layer.getId();
}

static const Layer& anyLayer(const QJsonValue& value, const Board& board) {
  const QString name = value.toString();
  if ((name == "top") || (name == "bottom") || name.startsWith("inner")) {
    return copperLayer(value, board);
  }
  return Layer::get(name);  // can throw
}

static QString padName(const BI_Pad& pad) noexcept {
  const PackagePad* libPad = pad.getLibPackagePad();
  return libPad ? *libPad->getName() : pad.getUuid().toStr();
}

static QString padRef(const BI_Pad& pad) noexcept {
  if (const BI_Device* dev = pad.getDevice()) {
    return *dev->getComponentInstance().getName() % "." % padName(pad);
  }
  return padName(pad);
}

static QString itemId(const QString& type, const Uuid& uuid) noexcept {
  return type % ":" % uuid.toStr();
}

static QJsonObject anchorToJson(const BI_NetLineAnchor& anchor) noexcept {
  QJsonObject obj{{"position", toJson(anchor.getPosition())}};
  if (const BI_Pad* pad = dynamic_cast<const BI_Pad*>(&anchor)) {
    obj.insert("pad", padRef(*pad));
  } else if (const BI_Via* via = dynamic_cast<const BI_Via*>(&anchor)) {
    obj.insert("via", itemId("via", via->getUuid()));
  } else if (const BI_NetPoint* p = dynamic_cast<const BI_NetPoint*>(&anchor)) {
    obj.insert("point", itemId("point", p->getUuid()));
  }
  return obj;
}

static BI_Pad& findPad(Board& board, const QString& ref) {
  const auto [designator, name] = splitPinRef(ref);
  ComponentInstance& cmp =
      findComponentInstance(board.getProject(), designator);
  BI_Device* dev = board.getDeviceInstanceByComponentUuid(cmp.getUuid());
  if (!dev) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Part '%1' is not placed on the board.").arg(designator));
  }
  BI_Pad* bySignal = nullptr;
  QStringList available;
  for (BI_Pad* pad : dev->getPads()) {
    if (padName(*pad) == name) {
      return *pad;
    }
    const ComponentSignalInstance* sig = pad->getComponentSignalInstance();
    if (sig && (*sig->getCompSignal().getName() == name)) {
      bySignal = pad;
    }
    available.append(padName(*pad));
  }
  if (bySignal) {
    return *bySignal;
  }
  available.sort();
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Part '%1' has no pad '%2'. Available: %3")
                         .arg(designator, name, available.join(", ")));
}

struct Anchor {
  BI_NetLineAnchor* anchor;
  BI_NetSegment* segment;
  NetSignal* net;
};

static Anchor findAnchor(Board& board, const QString& ref, const Layer& layer) {
  if (ref.startsWith("point:")) {
    const std::optional<Uuid> uuid = Uuid::tryFromString(ref.mid(6));
    for (BI_NetSegment* segment : board.getNetSegments()) {
      if (BI_NetPoint* p =
              uuid ? segment->getNetPoints().value(*uuid) : nullptr) {
        return Anchor{p, segment, segment->getNetSignal()};
      }
    }
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Junction '%1' not found.").arg(ref));
  }
  if (ref.startsWith("via:")) {
    const std::optional<Uuid> uuid = Uuid::tryFromString(ref.mid(4));
    for (BI_NetSegment* segment : board.getNetSegments()) {
      if (BI_Via* via = uuid ? segment->getVias().value(*uuid) : nullptr) {
        return Anchor{via, segment, segment->getNetSignal()};
      }
    }
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Via '%1' not found.").arg(ref));
  }
  BI_Pad& pad = findPad(board, ref);
  if (!pad.isOnLayer(layer)) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Pad '%1' is not on the %2 layer.")
                           .arg(ref, layer.getNameTr()));
  }
  BI_NetSegment* segment = pad.getNetSegmentOfLines();
  if (!segment) {
    segment = pad.getNetSegment();
  }
  return Anchor{&pad, segment, pad.getNetSignal()};
}

/**
 * @brief The junction, via or trace of a net at a position on a layer
 *
 * A trace is split at the position (projected onto the trace) to get a
 * junction to connect to.
 */
static Anchor anchorAt(Board& board, NetSignal& net, const Layer& layer,
                       const Point& pos, UndoStackTransaction& transaction) {
  for (BI_NetSegment* segment : board.getNetSegments()) {
    if (segment->getNetSignal() != &net) continue;
    for (BI_Via* via : segment->getVias()) {
      if ((via->getPosition() - pos).getLength() <=
          (*via->getActualSize() / 2)) {
        return Anchor{via, segment, &net};
      }
    }
    for (BI_NetPoint* p : segment->getNetPoints()) {
      const std::optional<PositiveLength> width = p->getMaxTraceWidth();
      if ((p->getLayerOfTraces() == &layer) && width &&
          ((p->getPosition() - pos).getLength() <= (**width / 2))) {
        return Anchor{p, segment, &net};
      }
    }
    for (BI_NetLine* line : segment->getNetLines()) {
      if (line->getLayer() != layer) continue;
      const Point p1 = line->getP1().getPosition();
      const Point p2 = line->getP2().getPosition();
      const Point nearest = Toolbox::nearestPointOnLine(pos, p1, p2);
      if ((nearest - pos).getLength() <= (*line->getWidth() / 2)) {
        if (nearest == p1) return Anchor{&line->getP1(), segment, &net};
        if (nearest == p2) return Anchor{&line->getP2(), segment, &net};
        auto* cmd = new editor::CmdBoardSplitNetLine(*line, nearest);
        transaction.append(cmd);  // can throw
        return Anchor{cmd->getSplitPoint(), segment, &net};
      }
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("No trace, junction or via of net '%1' on layer '%2' at [%3, "
              "%4].")
          .arg(*net.getName(), layerName(layer))
          .arg(pos.getX().toMm())
          .arg(pos.getY().toMm()));
}

static QSet<Uuid> traceUuids(const Board& board, const NetSignal& net) {
  QSet<Uuid> uuids;
  for (const BI_NetSegment* segment : board.getNetSegments()) {
    if (segment->getNetSignal() == &net) {
      uuids |= Toolbox::toSet(segment->getNetLines().keys());
    }
  }
  return uuids;
}

static QJsonObject deviceToJson(const BI_Device& dev) noexcept {
  QJsonArray pads;
  for (const BI_Pad* pad : dev.getPads()) {
    const ComponentSignalInstance* sig = pad->getComponentSignalInstance();
    const NetSignal* net = pad->getNetSignal();
    const bool top = pad->isOnLayer(Layer::topCopper());
    const bool bottom = pad->isOnLayer(Layer::botCopper());
    pads.append(QJsonObject{
        {"pad", padName(*pad)},
        {"signal",
         sig ? QJsonValue(*sig->getCompSignal().getName()) : QJsonValue()},
        {"net", net ? QJsonValue(*net->getName()) : QJsonValue()},
        {"position", toJson(pad->getPosition())},
        {"layers", (top && bottom) ? "both" : (top ? "top" : "bottom")},
    });
  }
  QJsonObject result = footprintGeometry(dev.getLibFootprint(), Transform(dev));
  result.insert("designator", *dev.getComponentInstance().getName());
  result.insert("device", *dev.getLibDevice().getNames().getDefaultValue());
  result.insert("footprint",
                *dev.getLibFootprint().getNames().getDefaultValue());
  result.insert("position", toJson(dev.getPosition()));
  result.insert("rotation", dev.getRotation().toDeg());
  result.insert("side", dev.getMirrored() ? "bottom" : "top");
  result.insert("pads", pads);
  return result;
}

static PositiveLength traceWidth(const QJsonObject& args, const Board& board,
                                 const NetSignal& net) {
  if (args.contains("width")) {
    return PositiveLength(lengthMm(args.value("width"), "width"));
  }
  if (const auto width = net.getNetClass().getDefaultTraceWidth()) {
    return *width;
  }
  return board.getDesignRules().getDefaultTraceWidth();
}

/*******************************************************************************
 *  Board Items
 ******************************************************************************/

static QJsonValue netName(const NetSignal* net) noexcept {
  return net ? QJsonValue(*net->getName()) : QJsonValue();
}

static QString connectStyleName(BI_Plane::ConnectStyle style) noexcept {
  switch (style) {
    case BI_Plane::ConnectStyle::Solid:
      return "solid";
    case BI_Plane::ConnectStyle::None:
      return "none";
    default:
      return "thermal";
  }
}

static BI_Plane::ConnectStyle connectStyle(const QJsonValue& value) noexcept {
  const QString name = value.toString("thermal");
  if (name == "solid") {
    return BI_Plane::ConnectStyle::Solid;
  } else if (name == "none") {
    return BI_Plane::ConnectStyle::None;
  }
  return BI_Plane::ConnectStyle::ThermalRelief;
}

/**
 * @brief Copper layers of a keep-out: 'top', 'bottom', 'both' or 'all'
 */
static QSet<const Layer*> zoneLayers(const QJsonValue& value,
                                     const Board& board) {
  const QString name = value.toString("both");
  QSet<const Layer*> layers;
  if (name == "both") {
    layers = {&Layer::topCopper(), &Layer::botCopper()};
  } else if (name == "all") {
    layers = board.getCopperLayers();
  } else {
    layers.insert(&copperLayer(value, board));
  }
  return layers;
}

static QJsonArray layerNames(const QSet<const Layer*>& layers) noexcept {
  QList<const Layer*> sorted = layers.values();
  std::sort(sorted.begin(), sorted.end(), [](const Layer* a, const Layer* b) {
    return a->getCopperNumber() < b->getCopperNumber();
  });
  QJsonArray array;
  for (const Layer* layer : sorted) {
    array.append(layerName(*layer));
  }
  return array;
}

/**
 * @brief Axis-aligned bounding box in mm, for area filters
 */
struct Box {
  qreal left;
  qreal bottom;
  qreal right;
  qreal top;

  static Box around(const Point& p, const Length& radius) noexcept {
    const qreal x = p.getX().toMm(), y = p.getY().toMm(), r = radius.toMm();
    return Box{x - r, y - r, x + r, y + r};
  }
  static Box of(const Path& path) noexcept {
    const QRectF px = path.toQPainterPathPx().boundingRect();
    return Box{
        Length::fromPx(px.left()).toMm(), -Length::fromPx(px.bottom()).toMm(),
        Length::fromPx(px.right()).toMm(), -Length::fromPx(px.top()).toMm()};
  }
  bool overlaps(const Box& other) const noexcept {
    return (left <= other.right) && (other.left <= right) &&
        (bottom <= other.top) && (other.bottom <= top);
  }
  Box united(const Box& other) const noexcept {
    return Box{std::min(left, other.left), std::min(bottom, other.bottom),
               std::max(right, other.right), std::max(top, other.top)};
  }
};

static QJsonObject boxToJson(const Box& box) noexcept {
  auto mm = [](qreal value) { return std::round(value * 1000) / 1000; };
  return QJsonObject{
      {"from", QJsonArray{mm(box.left), mm(box.bottom)}},
      {"to", QJsonArray{mm(box.right), mm(box.top)}},
  };
}

static QJsonObject traceToJson(const BI_NetLine& line) noexcept {
  return QJsonObject{
      {"id", itemId("trace", line.getUuid())},
      {"net", netName(line.getNetSegment().getNetSignal())},
      {"layer", layerName(line.getLayer())},
      {"width", line.getWidth()->toMm()},
      {"from", anchorToJson(line.getP1())},
      {"to", anchorToJson(line.getP2())},
  };
}

static QJsonObject viaToJson(const BI_Via& via) noexcept {
  return QJsonObject{
      {"id", itemId("via", via.getUuid())},
      {"net", netName(via.getNetSegment().getNetSignal())},
      {"position", toJson(via.getPosition())},
      {"drill", via.getActualDrillDiameter()->toMm()},
      {"size", via.getActualSize()->toMm()},
  };
}

static QJsonObject padToJson(const BI_Pad& pad) noexcept {
  const bool top = pad.isOnLayer(Layer::topCopper());
  const bool bottom = pad.isOnLayer(Layer::botCopper());
  return QJsonObject{
      {"pad", padRef(pad)},
      {"net", netName(pad.getNetSignal())},
      {"position", toJson(pad.getPosition())},
      {"rotation", pad.getRotation().toDeg()},
      {"size",
       QJsonArray{pad.getProperties().getWidth()->toMm(),
                  pad.getProperties().getHeight()->toMm()}},
      {"layers", (top && bottom) ? "both" : (top ? "top" : "bottom")},
  };
}

/**
 * @brief ID of a text, with the designator for texts of parts since those
 *        reuse the UUIDs of the library footprint
 */
static QString textId(const BI_StrokeText& text) noexcept {
  if (const BI_Device* dev = text.getDevice()) {
    return QString("text:%1:%2")
        .arg(*dev->getComponentInstance().getName(),
             text.getData().getUuid().toStr());
  }
  return itemId("text", text.getData().getUuid());
}

static QJsonObject textToJson(const BI_StrokeText& text) noexcept {
  const BoardStrokeTextData& data = text.getData();
  QJsonObject obj{
      {"id", textId(text)},
      {"text", data.getText()},
      {"layer", data.getLayer().getId()},
      {"position", toJson(data.getPosition())},
      {"rotation", data.getRotation().toDeg()},
      {"height", data.getHeight()->toMm()},
  };
  if (const BI_Device* dev = text.getDevice()) {
    obj.insert("part", *dev->getComponentInstance().getName());
  }
  return obj;
}

/**
 * @brief A board item addressed by an ID like `trace:<uuid>`
 */
struct BoardItem {
  QString type;
  BI_NetLine* trace = nullptr;
  BI_Via* via = nullptr;
  BI_NetPoint* point = nullptr;
  BI_Plane* plane = nullptr;
  BI_Zone* keepout = nullptr;
  BI_Polygon* polygon = nullptr;
  BI_StrokeText* text = nullptr;
  BI_Hole* hole = nullptr;
};

static const QStringList sItemTypes = {"trace",   "via",     "point", "plane",
                                       "keepout", "polygon", "text",  "hole"};

static BI_StrokeText* findText(Board& board, const QString& id) {
  const int colon = id.lastIndexOf(':');
  const QString designator = id.mid(5, std::max(colon - 5, 0));
  const std::optional<Uuid> uuid = Uuid::tryFromString(id.mid(colon + 1));
  if (!uuid) return nullptr;
  if (designator.isEmpty()) {
    if (BI_StrokeText* text = board.getStrokeTexts().value(*uuid)) {
      return text;
    }
  }
  QStringList partTextIds;
  for (BI_Device* dev : board.getDeviceInstances()) {
    for (BI_StrokeText* text : dev->getStrokeTexts()) {
      if (text->getData().getUuid() != *uuid) continue;
      if (*dev->getComponentInstance().getName() == designator) return text;
      partTextIds.append(textId(*text));
    }
  }
  if (designator.isEmpty() && (!partTextIds.isEmpty())) {
    partTextIds.sort();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Texts of parts have IDs with the designator: %1")
            .arg(partTextIds.join(", ")));
  }
  return nullptr;
}

static BoardItem findItem(Board& board, const QString& id) {
  const int colon = id.indexOf(':');
  BoardItem item{id.left(colon)};
  if (item.type == "text") {
    item.text = findText(board, id);  // can throw
    if (item.text) return item;
  }
  const std::optional<Uuid> uuid = Uuid::tryFromString(id.mid(colon + 1));
  if ((colon > 0) && uuid) {
    for (BI_NetSegment* segment : board.getNetSegments()) {
      if (item.type == "trace")
        item.trace = segment->getNetLines().value(*uuid);
      if (item.type == "via") item.via = segment->getVias().value(*uuid);
      if (item.type == "point")
        item.point = segment->getNetPoints().value(*uuid);
      if (item.trace || item.via || item.point) return item;
    }
    if (item.type == "plane") item.plane = board.getPlanes().value(*uuid);
    if (item.type == "keepout") item.keepout = board.getZones().value(*uuid);
    if (item.type == "polygon") item.polygon = board.getPolygons().value(*uuid);
    if (item.type == "hole") item.hole = board.getHoles().value(*uuid);
    if (item.plane || item.keepout || item.polygon || item.hole) {
      return item;
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("No board item '%1' found. IDs look like '<type>:<uuid>' with "
              "type %2 ('text:<designator>:<uuid>' for texts of parts), as "
              "returned by board_list_items.")
          .arg(id, sItemTypes.join(", ")));
}

/**
 * @brief Copper area of a trace, as marked by the DRC
 */
static Path traceArea(const BI_NetLine& line) noexcept {
  return Path::obround(line.getP1().getPosition(), line.getP2().getPosition(),
                       line.getWidth());
}

/**
 * @brief Clearance problems of new copper, found by a quick DRC and by
 *        checking the keep-out zones
 *
 * Planes are not refilled for the quick check, but they are refilled around
 * new copper anyway, so their messages are skipped.
 */
static QJsonArray copperViolations(
    Board& board, const QList<std::pair<const Layer*, Path>>& areas) {
  QJsonArray violations;
  auto touches = [&areas](const Path& location, const Layer* layer) {
    const QPainterPath px = location.toQPainterPathPx();
    for (const auto& [areaLayer, area] : areas) {
      if (((!layer) || (layer == areaLayer)) &&
          px.intersects(area.toQPainterPathPx())) {
        return true;
      }
    }
    return false;
  };
  BoardDesignRuleCheck drc;
  drc.start(board, board.getDrcSettings(), true);
  const BoardDesignRuleCheck::Result result = drc.waitForFinished();
  for (const auto& msg : result.messages) {
    if (msg->getApproval().toByteArray().contains("(plane ")) continue;
    for (const Path& location : msg->getLocations()) {
      if (touches(location, nullptr)) {
        violations.append(QJsonObject{
            {"message", msg->getMessage()},
            {"location", locationToJson(location)},
        });
        break;
      }
    }
  }
  auto checkZone = [&](const Path& outline, const QSet<const Layer*>& layers,
                       Zone::Rules rules, const QString& owner) {
    if (!rules.testFlag(Zone::Rule::NoCopper)) return;
    for (const Layer* layer : layers) {
      if (touches(outline, layer)) {
        violations.append(QJsonObject{
            {"message",
             QString("Copper on %1 inside the keep-out zone of %2")
                 .arg(layerName(*layer), owner)},
            {"location", locationToJson(outline)},
        });
        return;
      }
    }
  };
  for (const BI_Zone* zone : board.getZones()) {
    checkZone(zone->getData().getOutline(), zone->getData().getLayers(),
              zone->getData().getRules(),
              itemId("keepout", zone->getData().getUuid()));
  }
  for (const BI_Device* dev : board.getDeviceInstances()) {
    const Transform transform(*dev);
    for (const Zone& zone : dev->getLibFootprint().getZones()) {
      QSet<const Layer*> layers;
      if (zone.getLayers().testFlag(Zone::Layer::Top)) {
        layers.insert(&transform.map(Layer::topCopper()));
      }
      if (zone.getLayers().testFlag(Zone::Layer::Bottom)) {
        layers.insert(&transform.map(Layer::botCopper()));
      }
      if (zone.getLayers().testFlag(Zone::Layer::Inner)) {
        for (const Layer* layer : board.getCopperLayers()) {
          if (layer->isInner()) layers.insert(layer);
        }
      }
      checkZone(transform.map(zone.getOutline()), layers, zone.getRules(),
                *dev->getComponentInstance().getName());
    }
  }
  return violations;
}

/*******************************************************************************
 *  Board Settings
 ******************************************************************************/

using DrcSettings = BoardDesignRuleCheckSettings;

struct DrcField {
  QString key;
  QString description;
  const UnsignedLength& (DrcSettings::*get)() const;
  void (DrcSettings::*set)(const UnsignedLength&);
};

static const QList<DrcField> sDrcFields = {
    {"minCopperClearance", "Copper to copper of other nets",
     &DrcSettings::getMinCopperCopperClearance,
     &DrcSettings::setMinCopperCopperClearance},
    {"minCopperBoardClearance", "Copper to board edge",
     &DrcSettings::getMinCopperBoardClearance,
     &DrcSettings::setMinCopperBoardClearance},
    {"minCopperNpthClearance", "Copper to non-plated holes",
     &DrcSettings::getMinCopperNpthClearance,
     &DrcSettings::setMinCopperNpthClearance},
    {"minDrillDrillClearance", "Drill to drill",
     &DrcSettings::getMinDrillDrillClearance,
     &DrcSettings::setMinDrillDrillClearance},
    {"minDrillBoardClearance", "Drill to board edge",
     &DrcSettings::getMinDrillBoardClearance,
     &DrcSettings::setMinDrillBoardClearance},
    {"minCopperWidth", "Copper width", &DrcSettings::getMinCopperWidth,
     &DrcSettings::setMinCopperWidth},
    {"minPthAnnularRing", "Annular ring of plated holes",
     &DrcSettings::getMinPthAnnularRing, &DrcSettings::setMinPthAnnularRing},
    {"minPthDrill", "Drill diameter of plated holes",
     &DrcSettings::getMinPthDrillDiameter,
     &DrcSettings::setMinPthDrillDiameter},
    {"minNpthDrill", "Drill diameter of non-plated holes",
     &DrcSettings::getMinNpthDrillDiameter,
     &DrcSettings::setMinNpthDrillDiameter},
    {"minSilkscreenWidth", "Silkscreen line width",
     &DrcSettings::getMinSilkscreenWidth, &DrcSettings::setMinSilkscreenWidth},
    {"minSilkscreenTextHeight", "Silkscreen text height",
     &DrcSettings::getMinSilkscreenTextHeight,
     &DrcSettings::setMinSilkscreenTextHeight},
    {"minOutlineToolDiameter", "Milling tool diameter for inner corners",
     &DrcSettings::getMinOutlineToolDiameter,
     &DrcSettings::setMinOutlineToolDiameter},
};

static QJsonValue optionalMm(
    const std::optional<PositiveLength>& value) noexcept {
  return value ? QJsonValue((*value)->toMm()) : QJsonValue();
}

/**
 * @brief Names of the design rule presets, e.g. "Aisler: 2-Layer"
 */
static QList<std::tuple<QString, FilePath, Uuid>> drcPresets(
    DesignSession& session) {
  QList<std::tuple<QString, FilePath, Uuid>> presets;
  if (!session.hasWorkspace()) return presets;
  for (const WorkspaceLibraryDb::Organization& org :
       session.getWorkspace().getLibraryDb().getAllLatestOrganizations(
           {}, true, false)) {  // can throw
    for (const WorkspaceLibraryDb::PcbDesignRules& rules : org.pcbDesignRules) {
      presets.append(std::make_tuple(org.name % ": " % rules.name, org.elemDir,
                                     rules.uuid));
    }
  }
  return presets;
}

static HAlign hAlign(const QJsonValue& value) noexcept {
  const QString align = value.toString("left");
  if (align == "center") {
    return HAlign::center();
  } else if (align == "right") {
    return HAlign::right();
  }
  return HAlign::left();
}

static const QJsonObject sLayer{
    {"type", "string"},
    {"description",
     "Copper layer: 'top', 'bottom', or 'inner1' to 'innerN' if the board "
     "has inner layers"},
};

static const QJsonObject sBoard{
    {"type", QJsonArray{"string", "integer"}},
    {"description", "Board name or index, default: the first one"},
};

/*******************************************************************************
 *  Functions
 ******************************************************************************/

void addBoardTools(McpServer& server, DesignSession& session) noexcept {
  server.addTool(McpTool{
      "board_set_outline",
      "Replace the board outline, either by a rectangle from (0, 0) to "
      "(width, height) or by a polygon. Use board_add_cutout for holes and "
      "slots inside the board.",
      objectSchema({{"board", sBoard},
                    {"width", numberProp("Width in mm of a rectangle")},
                    {"height", numberProp("Height in mm of a rectangle")},
                    {"vertices", verticesProp("Polygon outline")}}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const bool rect = args.contains("width") || args.contains("height");
        if (rect == args.contains("vertices")) {
          throw RuntimeError(
              __FILE__, __LINE__,
              "Give either 'width' and 'height', or 'vertices'.");
        }
        const Path path = rect
            ? Path::rect(Point(0, 0),
                         Point(lengthMm(args.value("width"), "width"),
                               lengthMm(args.value("height"), "height")))
            : closedPathMm(args.value("vertices"), "vertices");
        for (BI_Polygon* polygon : board.getPolygons()) {
          if (&polygon->getData().getLayer() == &Layer::boardOutlines()) {
            auto cmd = std::make_unique<editor::CmdBoardPolygonEdit>(*polygon);
            cmd->setPath(path, false);
            session.execCmd(cmd.release());  // can throw
            return McpToolResult::text("Board outline changed.");
          }
        }
        throw RuntimeError(__FILE__, __LINE__,
                           "The board has no outline polygon.");
      },
  });

  server.addTool(McpTool{
      "board_add_cutout",
      "Add a cutout (a hole or slot milled into the board), either a polygon "
      "or a circle.",
      objectSchema({{"board", sBoard},
                    {"vertices", verticesProp("Polygon of the cutout")},
                    {"center", pointProp("Center of a round cutout")},
                    {"diameter", numberProp("Diameter of a round cutout")}}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        if (args.contains("vertices") == args.contains("center")) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Give either 'vertices', or 'center' and "
                             "'diameter'.");
        }
        Path path;
        if (args.contains("vertices")) {
          path = closedPathMm(args.value("vertices"), "vertices");
        } else {
          path =
              Path::circle(PositiveLength(lengthMm(args.value("diameter"),
                                                   "diameter")))  // can throw
                  .translated(pointMm(args.value("center"), "center"));
        }
        auto* polygon = new BI_Polygon(
            board,
            BoardPolygonData(Uuid::createRandom(), Layer::boardCutouts(),
                             UnsignedLength(0), path, false, false, false));
        session.execCmd(new editor::CmdBoardPolygonAdd(*polygon));  // can throw
        return McpToolResult::json(QJsonObject{
            {"polygon", itemId("polygon", polygon->getData().getUuid())}});
      },
  });

  server.addTool(McpTool{
      "board_place_device",
      "Place a part on the board, or move it if already placed. The device "
      "(and thus the package) is taken from the schematic part if chosen "
      "there, otherwise it must be given. Returns the pads with their "
      "positions and nets. board_query shows what occupies an area.",
      objectSchema(
          {{"board", sBoard},
           {"designator", stringProp("e.g. 'R1'")},
           {"device",
            stringProp("Name or UUID of the device, e.g. 'Capacitor "
                       "3216 (1206)'")},
           {"footprint", stringProp("Optional footprint name of the package")},
           {"position", pointProp("Position of the part origin")},
           {"rotation", numberProp("Rotation in degrees")},
           {"side", enumProp({"top", "bottom"}, "Board side, default top")}},
          {"designator", "position"}),
      false,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        Board& board = session.getBoard(args.value("board"));
        ComponentInstance& cmp =
            findComponentInstance(project, requiredString(args, "designator"));
        if (cmp.getLibComponent().isSchematicOnly()) {
          throw RuntimeError(__FILE__, __LINE__,
                             "This part exists only in the schematic.");
        }
        const Point position = pointMm(args.value("position"), "position");
        const Angle rotation = angleDeg(args.value("rotation"), "rotation");
        const bool bottom = (args.value("side").toString() == "bottom");
        BI_Device* dev = board.getDeviceInstanceByComponentUuid(cmp.getUuid());

        // Determine device and footprint, if needed.
        std::optional<Uuid> deviceUuid;
        std::optional<Uuid> footprintUuid;
        const QString deviceName = args.value("device").toString();
        const QString footprintName = args.value("footprint").toString();
        if ((!deviceName.isEmpty()) || (!footprintName.isEmpty()) || (!dev)) {
          Workspace& ws = session.getWorkspace();
          const WorkspaceLibraryDb& db = ws.getLibraryDb();
          FilePath devFp;
          if (!deviceName.isEmpty()) {
            std::tie(deviceUuid, devFp) =
                findLibraryElement<Device>(db, deviceName);
          } else if (dev) {
            deviceUuid = dev->getLibDevice().getUuid();
            devFp = db.getLatest<Device>(*deviceUuid);
          } else if (!cmp.getCompatibleDevices().isEmpty()) {
            deviceUuid = *cmp.getCompatibleDevices().begin();
            devFp = db.getLatest<Device>(*deviceUuid);
          } else {
            QStringList names;
            for (const Uuid& uuid :
                 db.getComponentDevices(cmp.getLibComponent().getUuid())) {
              QString name;
              db.getTranslations<Device>(db.getLatest<Device>(uuid), {}, &name);
              names.append("'" % name % "'");
            }
            names.sort();
            throw RuntimeError(
                __FILE__, __LINE__,
                QString("Choose a device for '%1'. Available: %2")
                    .arg(*cmp.getName(), names.join(", ")));
          }
          std::unique_ptr<Device> libDev = openElement<Device>(devFp);
          if (libDev->getComponentUuid() != cmp.getLibComponent().getUuid()) {
            throw RuntimeError(__FILE__, __LINE__,
                               "The device does not belong to the component.");
          }
          if (!footprintName.isEmpty()) {
            std::unique_ptr<Package> pkg = openElement<Package>(
                db.getLatest<Package>(libDev->getPackageUuid()));
            QStringList names;
            for (const Footprint& fpt : pkg->getFootprints()) {
              if (fpt.getNames().getDefaultValue()->compare(
                      footprintName, Qt::CaseInsensitive) == 0) {
                footprintUuid = fpt.getUuid();
              }
              names.append("'" % *fpt.getNames().getDefaultValue() % "'");
            }
            if (!footprintUuid) {
              throw RuntimeError(__FILE__, __LINE__,
                                 QString("Footprint not found. Available: %1")
                                     .arg(names.join(", ")));
            }
          }
        }

        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: board_place_device");
        if (dev && deviceUuid &&
            ((*deviceUuid != dev->getLibDevice().getUuid()) ||
             (footprintUuid &&
              (*footprintUuid != dev->getLibFootprint().getUuid())))) {
          transaction.append(new editor::CmdReplaceDevice(
              session.getWorkspace(), board, *dev, *deviceUuid,
              footprintUuid));  // can throw
          dev = board.getDeviceInstanceByComponentUuid(cmp.getUuid());
        }
        if (dev) {
          auto* cmd = new editor::CmdDeviceInstanceEditAll(*dev);
          cmd->setPosition(position, false);
          cmd->setRotation(rotation, false);
          cmd->setMirrored(bottom, board.getInnerLayerCount(), false);
          transaction.append(cmd);  // can throw
        } else {
          auto* cmd = new editor::CmdAddDeviceToBoard(
              session.getWorkspace(), board, cmp, *deviceUuid, footprintUuid,
              std::nullopt, position, rotation, bottom);
          transaction.append(cmd);  // can throw
          dev = cmd->getDeviceInstance();
        }
        transaction.commit();  // can throw
        return McpToolResult::json(deviceToJson(*dev));
      },
  });

  server.addTool(McpTool{
      "board_get_part",
      "Get device, footprint, placement, pads (position, net, layers), "
      "courtyard, keep-outs and holes of a part on the board, in board "
      "coordinates.",
      objectSchema({{"board", sBoard}, {"designator", stringProp("e.g. 'U1'")}},
                   {"designator"}),
      true,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        ComponentInstance& cmp = findComponentInstance(
            session.getProject(), requiredString(args, "designator"));
        BI_Device* dev = board.getDeviceInstanceByComponentUuid(cmp.getUuid());
        if (!dev) {
          throw RuntimeError(__FILE__, __LINE__,
                             "The part is not placed on the board.");
        }
        return McpToolResult::json(deviceToJson(*dev));
      },
  });

  auto routeEnd = [](const QString& description) {
    return QJsonObject{
        {"type", QJsonArray{"string", "array"}},
        {"items", QJsonObject{{"type", "number"}}},
        {"minItems", 2},
        {"maxItems", 2},
        {"description", description},
    };
  };
  server.addTool(McpTool{
      "board_route",
      "Draw a trace on one copper layer between two points of the same net, "
      "optionally through corners. An end is a pad ('<designator>.<pad>', "
      "e.g. 'U1.8'), a via or junction ID ('via:...', 'point:...'), or a "
      "position [x, y] on a trace, junction or via of the net on that layer. "
      "A trace there is split into two (new IDs) for a T-junction. To change "
      "the layer, end at a via. Without 'width', the default trace width of "
      "the net class or else of the board design rules is used (0.5 mm "
      "unless changed). Returns the IDs of the new traces, the width, and "
      "clearance violations of the new copper from a quick check (the trace "
      "is kept; use board_remove_items or undo to revert it). board_query "
      "shows which layers are free along a planned path.",
      objectSchema(
          {{"board", sBoard},
           {"from", routeEnd("Start, e.g. 'R1.1'")},
           {"to", routeEnd("End, e.g. 'U1.7' or [12.5, 8]")},
           {"layer", sLayer},
           {"points", arrayProp(pointProp("Corner"), "Optional corners")},
           {"width", numberProp("Optional trace width in mm")},
           {"check",
            boolProp("Check the clearances of the new copper, default true")}},
          {"from", "to", "layer"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const QJsonValue layerValue = args.value("layer");
        const Layer& layer = copperLayer(layerValue, board);
        const QJsonValue fromValue = args.value("from");
        const QJsonValue toValue = args.value("to");
        if (fromValue.isArray() && toValue.isArray()) {
          throw RuntimeError(__FILE__, __LINE__,
                             "At least one end must be a pad, via or "
                             "junction.");
        }
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: board_route");
        std::optional<Anchor> a, b;
        if (fromValue.isString()) {
          a = findAnchor(board, requiredString(args, "from"), layer);
        }
        if (toValue.isString()) {
          b = findAnchor(board, requiredString(args, "to"), layer);
        }
        NetSignal* net = a ? a->net : b->net;
        if ((!net) || (a && b && (!b->net))) {
          throw RuntimeError(__FILE__, __LINE__,
                             "Both ends must be connected to a net in the "
                             "schematic.");
        }
        if (a && b && (a->net != b->net)) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("The ends belong to different nets ('%1' and '%2').")
                  .arg(*a->net->getName(), *b->net->getName()));
        }
        if (!a) {
          a = anchorAt(board, *net, layer, pointMm(fromValue, "from"),
                       transaction);
        }
        if (!b) {
          b = anchorAt(board, *net, layer, pointMm(toValue, "to"), transaction);
        }
        const PositiveLength width = traceWidth(args, board, *net);
        const QSet<Uuid> oldTraces = traceUuids(board, *net);

        BI_NetSegment* segment = a->segment ? a->segment : b->segment;
        if (!segment) {
          auto* cmd = new editor::CmdBoardNetSegmentAdd(board, net);
          transaction.append(cmd);  // can throw
          segment = cmd->getNetSegment();
        }
        const bool combine =
            a->segment && b->segment && (a->segment != b->segment);
        auto* cmdAdd = new editor::CmdBoardNetSegmentAddElements(*segment);
        BI_NetLineAnchor* previous = a->anchor;
        for (const QJsonValue& value : args.value("points").toArray()) {
          BI_NetPoint* point = cmdAdd->addNetPoint(pointMm(value, "points"));
          cmdAdd->addNetLine(*previous, *point, layer, width);
          previous = point;
        }
        BI_NetPoint* temporary =
            combine ? cmdAdd->addNetPoint(b->anchor->getPosition()) : nullptr;
        cmdAdd->addNetLine(*previous, temporary ? *temporary : *b->anchor,
                           layer, width);
        transaction.append(cmdAdd);  // can throw
        if (combine) {
          transaction.append(new editor::CmdCombineBoardNetSegments(
              *segment, *temporary, *b->segment, *b->anchor));  // can throw
          segment = b->segment;
        }
        transaction.append(
            new editor::CmdSimplifyBoardNetSegments({segment}));  // can throw
        transaction.commit();  // can throw

        QJsonArray traces;
        QList<std::pair<const Layer*, Path>> areas;
        for (const BI_NetSegment* seg : board.getNetSegments()) {
          if (seg->getNetSignal() != net) continue;
          for (const BI_NetLine* line : seg->getNetLines()) {
            if (!oldTraces.contains(line->getUuid())) {
              traces.append(itemId("trace", line->getUuid()));
              areas.append(std::make_pair(&line->getLayer(), traceArea(*line)));
            }
          }
        }
        QJsonObject result{{"traces", traces}, {"width", width->toMm()}};
        if (args.value("check").toBool(true)) {
          result.insert("violations", copperViolations(board, areas));
        }
        return McpToolResult::json(result);
      },
  });

  server.addTool(McpTool{
      "board_add_via",
      "Add a through via (connecting all copper layers) to a net. Returns "
      "its ID for board_route and clearance violations from a quick check.",
      objectSchema(
          {{"board", sBoard},
           {"net", stringProp("Net name, e.g. 'GND'")},
           {"position", pointProp("Position")},
           {"drill",
            numberProp("Optional drill diameter in mm, default from the net "
                       "class or the design rules")},
           {"size",
            numberProp("Optional outer diameter in mm, default from the "
                       "annular ring design rule")},
           {"check",
            boolProp("Check the clearances of the via, default true")}},
          {"net", "position"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        NetSignal* net = session.getProject().getCircuit().getNetSignalByName(
            requiredString(args, "net"));
        if (!net) {
          throw RuntimeError(__FILE__, __LINE__, "Net not found.");
        }
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: board_add_via");
        auto* cmdSegment = new editor::CmdBoardNetSegmentAdd(board, net);
        transaction.append(cmdSegment);  // can throw
        auto* cmdAdd = new editor::CmdBoardNetSegmentAddElements(
            *cmdSegment->getNetSegment());
        auto optionalLength =
            [&args](const QString& key) -> std::optional<PositiveLength> {
          if (!args.contains(key)) return std::nullopt;
          return PositiveLength(lengthMm(args.value(key), key));  // can throw
        };
        BI_Via* via = cmdAdd->addVia(
            Via(Uuid::createRandom(), Layer::topCopper(), Layer::botCopper(),
                pointMm(args.value("position"), "position"),
                optionalLength("drill"), optionalLength("size"),
                MaskConfig::off()));
        transaction.append(cmdAdd);  // can throw
        transaction.commit();  // can throw
        QJsonObject result{{"via", itemId("via", via->getUuid())}};
        if (args.value("check").toBool(true)) {
          QList<std::pair<const Layer*, Path>> areas;
          for (const Layer* layer : board.getCopperLayers()) {
            areas.append(std::make_pair(layer,
                                        Path::circle(via->getActualSize())
                                            .translated(via->getPosition())));
          }
          result.insert("violations", copperViolations(board, areas));
        }
        return McpToolResult::json(result);
      },
  });

  server.addTool(McpTool{
      "board_add_plane",
      "Add a copper plane (filled area) of a net on a layer, covering the "
      "whole board or the given area. It is filled automatically around pads "
      "and traces of other nets and is clipped by keep-outs "
      "(board_add_keepout).",
      objectSchema(
          {{"board", sBoard},
           {"net", stringProp("Net name, e.g. 'GND'")},
           {"layer", sLayer},
           {"vertices", verticesProp("Optional area, default: whole board")},
           {"priority",
            numberProp("Optional priority, overlapping planes with higher "
                       "priority are filled first, default 0")},
           {"connectStyle",
            enumProp({"thermal", "solid", "none"},
                     "How pads are connected, default thermal")},
           {"clearance",
            numberProp("Optional clearance to other copper in mm")}},
          {"net", "layer"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        NetSignal* net = session.getProject().getCircuit().getNetSignalByName(
            requiredString(args, "net"));
        if (!net) {
          throw RuntimeError(__FILE__, __LINE__, "Net not found.");
        }
        const std::optional<std::pair<Point, Point>> bbox =
            board.calculateBoundingRect();
        if (!bbox) {
          throw RuntimeError(__FILE__, __LINE__,
                             "The board has no valid outline.");
        }
        const Point margin(Length(2000000), Length(2000000));
        const Path outline = args.contains("vertices")
            ? closedPathMm(args.value("vertices"), "vertices")
            : Path::rect(bbox->first - margin, bbox->second + margin);

        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: board_add_plane");
        BI_Plane* plane =
            new BI_Plane(board, Uuid::createRandom(),
                         copperLayer(args.value("layer"), board), net, outline);
        plane->setConnectStyle(connectStyle(args.value("connectStyle")));
        plane->setPriority(args.value("priority").toInt(0));
        transaction.append(new editor::CmdBoardPlaneAdd(*plane));  // can throw

        // Same settings as when adding planes in the board editor.
        auto clearance = [](const UnsignedLength& a, const UnsignedLength& b,
                            const UnsignedLength& fallback) {
          const UnsignedLength value = std::max(a, b);
          return (value > 0) ? value : fallback;
        };
        const NetClass& nc = net->getNetClass();
        const BoardDesignRuleCheckSettings& drc = board.getDrcSettings();
        UnsignedLength minWidth = positiveToUnsigned(
            nc.getDefaultTraceWidth()
                ? *nc.getDefaultTraceWidth()
                : board.getDesignRules().getDefaultTraceWidth());
        minWidth = std::min(minWidth, UnsignedLength(200000));
        minWidth = std::max(minWidth, drc.getMinCopperWidth());
        auto cmd = std::make_unique<editor::CmdBoardPlaneEdit>(*plane);
        cmd->setMinWidth(minWidth);
        cmd->setThermalSpokeWidth((minWidth > 0) ? PositiveLength(*minWidth)
                                                 : PositiveLength(200000));
        cmd->setMinClearanceToCopper(
            args.contains("clearance")
                ? UnsignedLength(lengthMm(args.value("clearance"), "clearance"))
                : clearance(nc.getMinCopperCopperClearance(),
                            drc.getMinCopperCopperClearance(),
                            UnsignedLength(250000)));
        cmd->setMinClearanceToBoard(clearance(nc.getMinCopperCopperClearance(),
                                              drc.getMinCopperBoardClearance(),
                                              UnsignedLength(300000)));
        cmd->setMinClearanceToNpth(clearance(nc.getMinCopperCopperClearance(),
                                             drc.getMinCopperNpthClearance(),
                                             UnsignedLength(300000)));
        transaction.append(cmd.release());  // can throw
        transaction.commit();  // can throw
        return McpToolResult::json(
            QJsonObject{{"plane", itemId("plane", plane->getUuid())}});
      },
  });

  server.addTool(McpTool{
      "board_add_keepout",
      "Add a keep-out zone where copper, planes, stop mask openings or "
      "devices are not allowed, e.g. below a PCB antenna or around a "
      "mounting hole.",
      objectSchema(
          {{"board", sBoard},
           {"vertices", verticesProp("Area of the zone")},
           {"layers",
            stringProp("Copper layers: 'both' (top and bottom, default), "
                       "'all' (including inner layers) or one layer like "
                       "'top'")},
           {"rules",
            arrayProp(enumProp(zoneRuleNames(), "Rule"),
                      "What is not allowed in the zone, default "
                      "['noCopper', 'noPlanes']. 'noCopper' applies to "
                      "traces, vias and pads, 'noExposure' to stop mask "
                      "openings.")}},
          {"vertices"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const Zone::Rules rules = zoneRules(
            args.contains("rules") ? args.value("rules")
                                   : QJsonArray{"noCopper", "noPlanes"});
        auto* zone = new BI_Zone(
            board,
            BoardZoneData(Uuid::createRandom(),
                          zoneLayers(args.value("layers"), board), rules,
                          closedPathMm(args.value("vertices"), "vertices"),
                          false));
        session.execCmd(new editor::CmdBoardZoneAdd(*zone));  // can throw
        return McpToolResult::json(QJsonObject{
            {"keepout", itemId("keepout", zone->getData().getUuid())}});
      },
  });

  server.addTool(McpTool{
      "board_add_text",
      "Add a text to the board, e.g. product name, version or connector "
      "labels on the silkscreen (legend). Texts on bottom layers are "
      "mirrored automatically. Placeholders like {{PROJECT}}, {{VERSION}} "
      "or {{BOARD}} are substituted.",
      objectSchema(
          {{"board", sBoard},
           {"text", stringProp("The text")},
           {"position", pointProp("Anchor of the text, see 'align'")},
           {"rotation", numberProp("Rotation in degrees")},
           {"height", numberProp("Text height in mm, default 1.0")},
           {"strokeWidth", numberProp("Line width in mm, default 0.15")},
           {"align",
            enumProp({"left", "center", "right"},
                     "Horizontal alignment at the anchor, default left. "
                     "The anchor is always at the bottom of the text.")},
           {"layer",
            enumProp(
                {"top_legend", "bot_legend", "top_cu", "bot_cu",
                 "top_documentation", "bot_documentation", "brd_documentation"},
                "Layer, default top_legend (top silkscreen)")}},
          {"text", "position"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const QString layerId = args.value("layer").toString("top_legend");
        const Layer& layer = Layer::get(layerId);
        auto* text = new BI_StrokeText(
            board,
            BoardStrokeTextData(
                Uuid::createRandom(), layer, requiredString(args, "text"),
                pointMm(args.value("position"), "position"),
                angleDeg(args.value("rotation"), "rotation"),
                PositiveLength(args.contains("height")
                                   ? lengthMm(args.value("height"), "height")
                                   : Length(1000000)),
                UnsignedLength(
                    args.contains("strokeWidth")
                        ? lengthMm(args.value("strokeWidth"), "strokeWidth")
                        : Length(150000)),
                StrokeTextSpacing(), StrokeTextSpacing(),
                Alignment(hAlign(args.value("align")), VAlign::bottom()),
                layer.isBottom(), true,
                false));  // can throw
        session.execCmd(new editor::CmdBoardStrokeTextAdd(*text));
        return McpToolResult::json(QJsonObject{
            {"text", QString("text:" % text->getData().getUuid().toStr())}});
      },
  });

  server.addTool(McpTool{
      "board_set_part_text",
      "Move, rotate, resize, hide or show the designator ('name') or value "
      "text of placed parts, e.g. to keep it off pads and other parts. "
      "Several parts can be given to change them alike (e.g. hide all "
      "values).",
      objectSchema({{"board", sBoard},
                    {"designator",
                     QJsonObject{
                         {"type", QJsonArray{"string", "array"}},
                         {"items", QJsonObject{{"type", "string"}}},
                         {"description", "e.g. 'R1' or ['R1', 'R2']"},
                     }},
                    {"text", enumProp({"name", "value"}, "Which text")},
                    {"position", pointProp("New anchor position")},
                    {"rotation", numberProp("New rotation in degrees")},
                    {"height", numberProp("New text height in mm")},
                    {"align",
                     enumProp({"left", "center", "right"},
                              "New horizontal alignment at the anchor, which "
                              "is then at the bottom of the text as with "
                              "board_add_text. Footprint texts are often "
                              "centered on their anchor.")},
                    {"visible", boolProp("Show or remove the text")}},
                   {"designator", "text"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const QJsonValue designators = args.value("designator");
        QList<BI_Device*> devices;
        for (const QJsonValue& designator : designators.isArray()
                 ? designators.toArray()
                 : QJsonArray{designators}) {
          ComponentInstance& cmp = findComponentInstance(session.getProject(),
                                                         designator.toString());
          BI_Device* dev =
              board.getDeviceInstanceByComponentUuid(cmp.getUuid());
          if (!dev) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("The part '%1' is not placed on the "
                                       "board.")
                                   .arg(*cmp.getName()));
          }
          devices.append(dev);
        }
        const QString placeholder =
            (args.value("text") == "value") ? "{{VALUE}}" : "{{NAME}}";
        const bool visible = args.value("visible").toBool(true);
        UndoStackTransaction transaction(session.getUndoStack(),
                                         "MCP: board_set_part_text");
        for (BI_Device* dev : devices) {
          QList<BI_StrokeText*> texts;
          for (BI_StrokeText* text : dev->getStrokeTexts()) {
            if (text->getData().getText() == placeholder) {
              texts.append(text);
            }
          }
          if (visible && texts.isEmpty()) {
            for (const StrokeText& text : dev->getDefaultStrokeTexts()) {
              if (text.getText() == placeholder) {
                auto* t = new BI_StrokeText(
                    board,
                    BoardStrokeTextData(
                        Uuid::createRandom(), text.getLayer(), text.getText(),
                        text.getPosition(), text.getRotation(),
                        text.getHeight(), text.getStrokeWidth(),
                        text.getLetterSpacing(), text.getLineSpacing(),
                        text.getAlign(), text.getMirrored(),
                        text.getAutoRotate(), false));
                transaction.append(
                    new editor::CmdDeviceStrokeTextAdd(*dev, *t));  // can throw
                texts.append(t);
              }
            }
          }
          for (BI_StrokeText* text : texts) {
            if (!visible) {
              transaction.append(
                  new editor::CmdDeviceStrokeTextRemove(*dev, *text));
              continue;
            }
            auto* cmd = new editor::CmdBoardStrokeTextEdit(*text);
            if (args.contains("position")) {
              cmd->setPosition(pointMm(args.value("position"), "position"),
                               false);
            }
            if (args.contains("rotation")) {
              cmd->setRotation(angleDeg(args.value("rotation"), "rotation"),
                               false);
            }
            if (args.contains("height")) {
              cmd->setHeight(PositiveLength(lengthMm(args.value("height"),
                                                     "height")),  // can throw
                             false);
            }
            if (args.contains("align")) {
              cmd->setAlignment(
                  Alignment(hAlign(args.value("align")), VAlign::bottom()),
                  false);
            }
            transaction.append(cmd);  // can throw
          }
        }
        transaction.commit();  // can throw
        return McpToolResult::text(visible ? "Text changed." : "Text removed.");
      },
  });

  server.addTool(McpTool{
      "board_add_hole",
      "Add a non-plated hole (NPTH), e.g. for a mounting screw without a "
      "footprint.",
      objectSchema({{"board", sBoard},
                    {"position", pointProp("Center")},
                    {"diameter", numberProp("Diameter in mm")}},
                   {"position", "diameter"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        auto* hole = new BI_Hole(
            board,
            BoardHoleData(
                Uuid::createRandom(),
                PositiveLength(lengthMm(args.value("diameter"), "diameter")),
                makeNonEmptyPath(pointMm(args.value("position"), "position")),
                MaskConfig::automatic(), false));  // can throw
        session.execCmd(new editor::CmdBoardHoleAdd(*hole));  // can throw
        return McpToolResult::json(
            QJsonObject{{"hole", itemId("hole", hole->getData().getUuid())}});
      },
  });

  const QStringList listTypes = {"traces", "vias",     "points",
                                 "planes", "keepouts", "polygons",
                                 "texts",  "holes",    "pads"};
  server.addTool(McpTool{
      "board_list_items",
      "List the items on a board with their IDs (for board_modify_item, "
      "board_remove_items and board_route), optionally only some types, "
      "those of a net, on a layer or overlapping an area. Types: traces, "
      "vias, points (junctions and corners of traces), planes, keepouts, "
      "polygons (board outline, cutouts, drawings), texts (including the "
      "name/value texts of parts), holes (NPTH) and pads.",
      objectSchema(
          {{"board", sBoard},
           {"types",
            arrayProp(enumProp(listTypes, "Item type"),
                      "Item types to list, default all except pads and "
                      "points")},
           {"net", stringProp("Only items of this net")},
           {"layer",
            stringProp("Only items on this layer: a copper layer ('top', "
                       "'bottom', 'inner1', ...) or a layer ID like "
                       "'top_legend' or 'brd_cutouts'")},
           {"area",
            objectSchema({{"from", pointProp("One corner")},
                          {"to", pointProp("Opposite corner")}},
                         {"from", "to"})}}),
      true,
      [&session, listTypes](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        QSet<QString> types;
        for (const QJsonValue& type : args.value("types").toArray()) {
          types.insert(type.toString());
        }
        if (types.isEmpty()) {
          types = Toolbox::toSet(listTypes);
          types.remove("pads");
          types.remove("points");
        }
        const NetSignal* net = nullptr;
        if (args.contains("net")) {
          net = session.getProject().getCircuit().getNetSignalByName(
              requiredString(args, "net"));
          if (!net) {
            throw RuntimeError(__FILE__, __LINE__, "Net not found.");
          }
        }
        const Layer* layer = args.contains("layer")
            ? &anyLayer(args.value("layer"), board)
            : nullptr;
        std::optional<Box> area;
        if (args.contains("area")) {
          const QJsonObject obj = args.value("area").toObject();
          const Point a = pointMm(obj.value("from"), "area.from");
          const Point b = pointMm(obj.value("to"), "area.to");
          area = Box{std::min(a.getX(), b.getX()).toMm(),
                     std::min(a.getY(), b.getY()).toMm(),
                     std::max(a.getX(), b.getX()).toMm(),
                     std::max(a.getY(), b.getY()).toMm()};
        }
        auto inArea = [&area](const Box& box) {
          return (!area) || area->overlaps(box);
        };
        auto netOk = [net](const NetSignal* itemNet, bool hasNet) {
          return (!net) || (hasNet && (itemNet == net));
        };
        auto layerOk = [layer](const Layer* itemLayer) {
          return (!layer) || (itemLayer == layer);
        };
        QMap<QString, QJsonArray> items;
        for (const BI_NetSegment* segment : board.getNetSegments()) {
          const NetSignal* segNet = segment->getNetSignal();
          if (!netOk(segNet, true)) continue;
          for (const BI_NetLine* line : segment->getNetLines()) {
            if (layerOk(&line->getLayer()) &&
                inArea(Box::of(traceArea(*line)))) {
              items["traces"].append(traceToJson(*line));
            }
          }
          for (const BI_Via* via : segment->getVias()) {
            if (((!layer) || layer->isCopper()) &&
                inArea(Box::around(via->getPosition(),
                                   *via->getActualSize() / 2))) {
              items["vias"].append(viaToJson(*via));
            }
          }
          for (const BI_NetPoint* p : segment->getNetPoints()) {
            if (layerOk(p->getLayerOfTraces()) &&
                inArea(Box::around(p->getPosition(), Length(0)))) {
              items["points"].append(QJsonObject{
                  {"id", itemId("point", p->getUuid())},
                  {"net", netName(segNet)},
                  {"position", toJson(p->getPosition())},
              });
            }
          }
        }
        for (const BI_Plane* plane : board.getPlanes()) {
          if (netOk(plane->getNetSignal(), true) &&
              layerOk(&plane->getLayer()) &&
              inArea(Box::of(plane->getOutline()))) {
            items["planes"].append(QJsonObject{
                {"id", itemId("plane", plane->getUuid())},
                {"net", netName(plane->getNetSignal())},
                {"layer", layerName(plane->getLayer())},
                {"priority", plane->getPriority()},
                {"connectStyle", connectStyleName(plane->getConnectStyle())},
                {"clearance", plane->getMinClearanceToCopper()->toMm()},
                {"vertices", verticesToJson(plane->getOutline())},
            });
          }
        }
        for (const BI_Zone* zone : board.getZones()) {
          const BoardZoneData& data = zone->getData();
          if (netOk(nullptr, false) &&
              ((!layer) || data.getLayers().contains(layer)) &&
              inArea(Box::of(data.getOutline()))) {
            items["keepouts"].append(QJsonObject{
                {"id", itemId("keepout", data.getUuid())},
                {"layers", layerNames(data.getLayers())},
                {"rules", zoneRulesToJson(data.getRules())},
                {"vertices", verticesToJson(data.getOutline())},
            });
          }
        }
        for (const BI_Polygon* polygon : board.getPolygons()) {
          const BoardPolygonData& data = polygon->getData();
          if (netOk(nullptr, false) && layerOk(&data.getLayer()) &&
              inArea(Box::of(data.getPath()))) {
            items["polygons"].append(QJsonObject{
                {"id", itemId("polygon", data.getUuid())},
                {"layer", data.getLayer().getId()},
                {"width", data.getLineWidth()->toMm()},
                {"filled", data.isFilled()},
                {"vertices", verticesToJson(data.getPath())},
            });
          }
        }
        QList<const BI_StrokeText*> texts;
        for (const BI_StrokeText* text : board.getStrokeTexts()) {
          texts.append(text);
        }
        for (const BI_Device* dev : board.getDeviceInstances()) {
          for (const BI_StrokeText* text : dev->getStrokeTexts()) {
            texts.append(text);
          }
        }
        for (const BI_StrokeText* text : texts) {
          if (netOk(nullptr, false) && layerOk(&text->getData().getLayer()) &&
              inArea(Box::around(text->getData().getPosition(), Length(0)))) {
            items["texts"].append(textToJson(*text));
          }
        }
        for (const BI_Hole* hole : board.getHoles()) {
          const BoardHoleData& data = hole->getData();
          const Point pos = data.getPath()->getVertices().first().getPos();
          if (netOk(nullptr, false) && (!layer) &&
              inArea(Box::around(pos, *data.getDiameter() / 2))) {
            items["holes"].append(QJsonObject{
                {"id", itemId("hole", data.getUuid())},
                {"position", toJson(pos)},
                {"diameter", data.getDiameter()->toMm()},
            });
          }
        }
        for (const BI_Device* dev : board.getDeviceInstances()) {
          for (const BI_Pad* pad : dev->getPads()) {
            const Length radius = std::max(*pad->getProperties().getWidth(),
                                           *pad->getProperties().getHeight()) /
                2;
            if (netOk(pad->getNetSignal(), true) &&
                ((!layer) || pad->isOnLayer(*layer)) &&
                inArea(Box::around(pad->getPosition(), radius))) {
              items["pads"].append(padToJson(*pad));
            }
          }
        }
        QJsonObject result;
        for (const QString& type : types) {
          result.insert(type, items.value(type));
        }
        return McpToolResult::json(result);
      },
  });

  server.addTool(McpTool{
      "board_query",
      "Show what is inside a rectangle of the board, to find free space for "
      "parts, traces and vias before placing them: the parts reaching into "
      "it (with their courtyard box and the pads inside), the traces "
      "passing through it as whole paths (from end to end, with the trace "
      "IDs in order), vias, holes, keep-outs (also those of parts), texts "
      "with their box and the planes covering it. Items count when their "
      "actual shape overlaps the rectangle. 'free' tells for each copper "
      "layer whether the rectangle holds no pad, trace or via of any net "
      "(planes keep their clearance by themselves), 'withinBoard' whether "
      "it lies inside the board outline and off cutouts. To check whether "
      "a trace or via fits, query the area it would cover, enlarged by the "
      "clearance, with its net. Keep the rectangle small, the whole board "
      "gives a long answer.",
      objectSchema(
          {{"board", sBoard},
           {"from", pointProp("One corner")},
           {"to", pointProp("Opposite corner")},
           {"net",
            stringProp("Optional net whose own copper does not count as "
                       "blocking in 'free'")}},
          {"from", "to"}),
      true,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const Point a = pointMm(args.value("from"), "from");
        const Point b = pointMm(args.value("to"), "to");
        const Path rect = Path::rect(a, b);
        const Box rectBox = Box::of(rect);
        const QPainterPath rectPx = rect.toQPainterPathPx();
        auto hits = [&rectPx](const Path& area) {
          return rectPx.intersects(area.toQPainterPathPx());
        };
        const NetSignal* net = nullptr;
        if (args.contains("net")) {
          net = session.getProject().getCircuit().getNetSignalByName(
              requiredString(args, "net"));
          if (!net) {
            throw RuntimeError(__FILE__, __LINE__, "Net not found.");
          }
        }
        auto blocks = [net](const NetSignal* itemNet) {
          return (!net) || (itemNet != net);
        };
        QSet<const Layer*> usedLayers;

        QList<const BI_Device*> devices;
        for (const BI_Device* dev : board.getDeviceInstances()) {
          devices.append(dev);
        }
        std::sort(devices.begin(), devices.end(),
                  [](const BI_Device* x, const BI_Device* y) {
                    return *x->getComponentInstance().getName() <
                        *y->getComponentInstance().getName();
                  });
        QJsonArray parts, keepouts;
        for (const BI_Device* dev : devices) {
          const QString designator = *dev->getComponentInstance().getName();
          const Transform transform(*dev);
          const Footprint& footprint = dev->getLibFootprint();
          QVector<Path> courtyard;
          for (const Polygon& polygon : footprint.getPolygons()) {
            const Layer& layer = transform.map(polygon.getLayer());
            if ((layer == Layer::topCourtyard()) ||
                (layer == Layer::botCourtyard())) {
              courtyard.append(transform.map(polygon.getPath()));
            }
          }
          for (const Circle& circle : footprint.getCircles()) {
            const Layer& layer = transform.map(circle.getLayer());
            if ((layer == Layer::topCourtyard()) ||
                (layer == Layer::botCourtyard())) {
              courtyard.append(
                  Path::circle(circle.getDiameter())
                      .translated(transform.map(circle.getCenter())));
            }
          }
          QJsonArray pads;
          std::optional<Box> box;
          for (const BI_Pad* pad : dev->getPads()) {
            const Transform padTransform(*pad);
            bool padHit = false;
            const auto& geometries = pad->getGeometries();
            for (auto it = geometries.begin(); it != geometries.end(); ++it) {
              if (!it.key()->isCopper()) continue;
              for (const PadGeometry& geometry : it.value()) {
                for (const Path& outline : geometry.toOutlines()) {
                  const Path area = padTransform.map(outline);
                  if (courtyard.isEmpty()) {
                    box = box ? box->united(Box::of(area)) : Box::of(area);
                  }
                  if (hits(area)) {
                    if (blocks(pad->getNetSignal())) {
                      usedLayers.insert(it.key());
                    }
                    padHit = true;
                  }
                }
              }
            }
            if (padHit) pads.append(padToJson(*pad));
          }
          bool partHit = !pads.isEmpty();
          for (const Path& area : courtyard) {
            box = box ? box->united(Box::of(area)) : Box::of(area);
            partHit = partHit || hits(area);
          }
          if (partHit) {
            QJsonObject part{
                {"designator", designator},
                {"position", toJson(dev->getPosition())},
                {"rotation", dev->getRotation().toDeg()},
                {"side", dev->getMirrored() ? "bottom" : "top"},
                {"pads", pads},
            };
            if (box) part.insert("box", boxToJson(*box));
            parts.append(part);
          }
          for (const Zone& zone : footprint.getZones()) {
            if (hits(transform.map(zone.getOutline()))) {
              keepouts.append(QJsonObject{
                  {"part", designator},
                  {"layers",
                   footprintZoneLayers(zone, transform.getMirrored())},
                  {"rules", zoneRulesToJson(zone.getRules())},
              });
            }
          }
        }

        auto next = [](const BI_NetLine& line,
                       const BI_NetLineAnchor& at) -> const BI_NetLine* {
          const BI_NetPoint* point = dynamic_cast<const BI_NetPoint*>(&at);
          if ((!point) || (point->getNetLines().count() != 2)) {
            return nullptr;
          }
          for (const BI_NetLine* other : point->getNetLines()) {
            if ((other != &line) && (&other->getLayer() == &line.getLayer()) &&
                (other->getWidth() == line.getWidth())) {
              return other;
            }
          }
          return nullptr;
        };
        auto otherEnd =
            [](const BI_NetLine& line,
               const BI_NetLineAnchor& at) -> const BI_NetLineAnchor& {
          return (&line.getP1() == &at) ? line.getP2() : line.getP1();
        };
        QJsonArray paths, vias;
        QSet<const BI_NetLine*> done;
        for (const BI_NetSegment* segment : board.getNetSegments()) {
          for (const BI_NetLine* seed : segment->getNetLines()) {
            if (done.contains(seed) || (!hits(traceArea(*seed)))) continue;
            const BI_NetLine* first = seed;
            const BI_NetLineAnchor* start = &seed->getP1();
            for (int i = 0; i < segment->getNetLines().count(); ++i) {
              const BI_NetLine* previous = next(*first, *start);
              if ((!previous) || (previous == seed)) break;
              start = &otherEnd(*previous, *start);
              first = previous;
            }
            QJsonArray points{toJson(start->getPosition())};
            QJsonArray traces;
            const BI_NetLine* line = first;
            const BI_NetLineAnchor* end = start;
            while (line && (!done.contains(line))) {
              done.insert(line);
              if (blocks(segment->getNetSignal()) && hits(traceArea(*line))) {
                usedLayers.insert(&line->getLayer());
              }
              traces.append(itemId("trace", line->getUuid()));
              end = &otherEnd(*line, *end);
              points.append(toJson(end->getPosition()));
              line = next(*line, *end);
            }
            paths.append(QJsonObject{
                {"net", netName(segment->getNetSignal())},
                {"layer", layerName(first->getLayer())},
                {"width", first->getWidth()->toMm()},
                {"from", anchorToJson(*start)},
                {"to", anchorToJson(*end)},
                {"points", points},
                {"traces", traces},
            });
          }
          for (const BI_Via* via : segment->getVias()) {
            if (hits(Path::circle(via->getActualSize())
                         .translated(via->getPosition()))) {
              vias.append(viaToJson(*via));
              for (const Layer* layer : board.getCopperLayers()) {
                if (blocks(segment->getNetSignal()) &&
                    via->getVia().isOnLayer(*layer)) {
                  usedLayers.insert(layer);
                }
              }
            }
          }
        }

        QJsonArray holes;
        for (const BI_Hole* hole : board.getHoles()) {
          const BoardHoleData& data = hole->getData();
          const QVector<Vertex>& vertices = data.getPath()->getVertices();
          bool hit = hits(Path::circle(data.getDiameter())
                              .translated(vertices.first().getPos()));
          for (int i = 1; i < vertices.count(); ++i) {
            hit = hit ||
                hits(Path::obround(vertices.at(i - 1).getPos(),
                                   vertices.at(i).getPos(),
                                   data.getDiameter()));
          }
          if (hit) {
            holes.append(QJsonObject{
                {"id", itemId("hole", data.getUuid())},
                {"position", toJson(vertices.first().getPos())},
                {"diameter", data.getDiameter()->toMm()},
            });
          }
        }
        for (const BI_Zone* zone : board.getZones()) {
          const BoardZoneData& data = zone->getData();
          if (hits(data.getOutline())) {
            keepouts.append(QJsonObject{
                {"id", itemId("keepout", data.getUuid())},
                {"layers", layerNames(data.getLayers())},
                {"rules", zoneRulesToJson(data.getRules())},
            });
          }
        }
        QJsonArray planes;
        for (const BI_Plane* plane : board.getPlanes()) {
          if (hits(plane->getOutline())) {
            planes.append(QJsonObject{
                {"id", itemId("plane", plane->getUuid())},
                {"net", netName(plane->getNetSignal())},
                {"layer", layerName(plane->getLayer())},
            });
          }
        }
        QPainterPath outline;
        bool offCutouts = true;
        for (const BI_Polygon* polygon : board.getPolygons()) {
          const BoardPolygonData& data = polygon->getData();
          if (data.getLayer() == Layer::boardOutlines()) {
            outline.addPath(data.getPath().toQPainterPathPx());
          } else if (data.getLayer() == Layer::boardCutouts()) {
            offCutouts = offCutouts && (!hits(data.getPath()));
          } else if (data.getLayer().isCopper() && hits(data.getPath())) {
            usedLayers.insert(&data.getLayer());
          }
        }

        QList<const BI_StrokeText*> allTexts;
        for (const BI_StrokeText* text : board.getStrokeTexts()) {
          allTexts.append(text);
        }
        for (const BI_Device* dev : devices) {
          for (const BI_StrokeText* text : dev->getStrokeTexts()) {
            allTexts.append(text);
          }
        }
        QJsonArray texts;
        for (const BI_StrokeText* text : allTexts) {
          const BoardStrokeTextData& data = text->getData();
          std::optional<Box> box;
          for (const Path& path : text->getPaths()) {
            const Path placed =
                (data.getMirrored() ? path.mirrored(Qt::Horizontal) : path)
                    .translated(data.getPosition());
            box = box ? box->united(Box::of(placed)) : Box::of(placed);
          }
          if (!box) continue;
          const qreal stroke = data.getStrokeWidth()->toMm() / 2;
          box = Box{box->left - stroke, box->bottom - stroke,
                    box->right + stroke, box->top + stroke};
          if (box->overlaps(rectBox)) {
            QJsonObject obj = textToJson(*text);
            obj.insert("box", boxToJson(*box));
            texts.append(obj);
            if (data.getLayer().isCopper()) usedLayers.insert(&data.getLayer());
          }
        }

        QJsonObject free;
        for (const Layer* layer : board.getCopperLayers()) {
          free.insert(layerName(*layer), !usedLayers.contains(layer));
        }
        QJsonObject result{
            {"area", boxToJson(rectBox)},
            {"withinBoard", offCutouts && outline.contains(rectPx)},
            {"free", free},
        };
        const QList<std::pair<QString, QJsonArray>> lists = {
            {"parts", parts},  {"paths", paths},       {"vias", vias},
            {"holes", holes},  {"keepouts", keepouts}, {"texts", texts},
            {"planes", planes}};
        for (const auto& [key, list] : lists) {
          if (!list.isEmpty()) result.insert(key, list);
        }
        return McpToolResult::json(result);
      },
  });

  server.addTool(McpTool{
      "board_remove_items",
      "Remove board items by their IDs (see board_list_items). 'net:<name>' "
      "removes all traces and vias of a net. Removing a junction removes its "
      "traces. Pads are removed with their part (part_remove).",
      objectSchema(
          {{"board", sBoard},
           {"ids",
            QJsonObject{
                {"type", "array"},
                {"items", QJsonObject{{"type", "string"}}},
                {"minItems", 1},
                {"description", "IDs like 'trace:<uuid>', or 'net:<name>'"},
            }}},
          {"ids"}),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        auto cmd = std::make_unique<editor::CmdRemoveBoardItems>(board);
        QSet<BI_NetLine*> lines;
        QSet<BI_Via*> vias;
        QSet<BI_NetPoint*> points;
        QSet<BI_Plane*> planes;
        QSet<BI_Zone*> zones;
        QSet<BI_Polygon*> polygons;
        QSet<BI_StrokeText*> texts;
        QSet<BI_Hole*> holes;
        int count = 0;
        for (const QJsonValue& value : args.value("ids").toArray()) {
          const QString id = value.toString();
          if (id.startsWith("net:")) {
            const NetSignal* net =
                session.getProject().getCircuit().getNetSignalByName(id.mid(4));
            if (!net) {
              throw RuntimeError(__FILE__, __LINE__,
                                 QString("Net '%1' not found.").arg(id.mid(4)));
            }
            for (BI_NetSegment* segment : board.getNetSegments()) {
              if (segment->getNetSignal() == net) {
                lines |= Toolbox::toSet(segment->getNetLines().values());
                vias |= Toolbox::toSet(segment->getVias().values());
                points |= Toolbox::toSet(segment->getNetPoints().values());
                count +=
                    segment->getNetLines().count() + segment->getVias().count();
              }
            }
            continue;
          }
          const BoardItem item = findItem(board, id);
          if (item.polygon &&
              (item.polygon->getData().getLayer() == Layer::boardOutlines())) {
            throw RuntimeError(__FILE__, __LINE__,
                               "The board outline cannot be removed, change "
                               "it with board_set_outline.");
          }
          if (item.trace) lines.insert(item.trace);
          if (item.via) vias.insert(item.via);
          if (item.point) {
            points.insert(item.point);
            lines |= item.point->getNetLines();
          }
          if (item.plane) planes.insert(item.plane);
          if (item.keepout) zones.insert(item.keepout);
          if (item.polygon) polygons.insert(item.polygon);
          if (item.text) texts.insert(item.text);
          if (item.hole) holes.insert(item.hole);
          ++count;
        }
        cmd->removeNetLines(lines);
        cmd->removeVias(vias);
        cmd->removeNetPoints(points);
        cmd->removePlanes(planes);
        cmd->removeZones(zones);
        cmd->removePolygons(polygons);
        cmd->removeStrokeTexts(texts);
        cmd->removeHoles(holes);
        session.execCmd(cmd.release());  // can throw
        return McpToolResult::json(QJsonObject{{"removed", count}});
      },
  });

  const QHash<QString, QStringList> modifiable = {
      {"trace", {"width", "layer"}},
      {"via", {"position", "drill", "size"}},
      {"point", {"position"}},
      {"plane",
       {"vertices", "layer", "net", "priority", "connectStyle", "clearance"}},
      {"keepout", {"vertices", "layers", "rules"}},
      {"polygon", {"vertices"}},
      {"text", {"text", "position", "rotation", "height", "align", "layer"}},
      {"hole", {"position", "diameter"}},
  };
  server.addTool(McpTool{
      "board_modify_item",
      "Change a board item given by its ID (see board_list_items). Fields per "
      "type: trace: width, layer. via: position, drill, size. point "
      "(junction or corner): position. plane: vertices, layer, net, "
      "priority, connectStyle, clearance. keepout: vertices, layers, rules. "
      "polygon: vertices. text: text, position, rotation, height, align, "
      "layer. hole: position, diameter. Traces follow moved vias and points.",
      objectSchema(
          {{"board", sBoard},
           {"id", stringProp("Item ID, e.g. 'via:<uuid>'")},
           {"position", pointProp("New position")},
           {"width", numberProp("New trace width in mm")},
           {"layer",
            stringProp("New layer: copper layer name for traces and planes, "
                       "layer ID like 'top_legend' for texts")},
           {"drill", numberProp("New via drill diameter in mm")},
           {"size", numberProp("New via outer diameter in mm")},
           {"vertices", verticesProp("New outline")},
           {"net", stringProp("New net of a plane")},
           {"priority", numberProp("New plane priority")},
           {"connectStyle",
            enumProp({"thermal", "solid", "none"}, "New pad connection")},
           {"clearance", numberProp("New plane clearance in mm")},
           {"layers",
            stringProp("New keep-out layers: 'both', 'all' or one layer")},
           {"rules", arrayProp(enumProp(zoneRuleNames(), "Rule"), "New rules")},
           {"text", stringProp("New text")},
           {"rotation", numberProp("New rotation in degrees")},
           {"height", numberProp("New text height in mm")},
           {"align",
            enumProp({"left", "center", "right"},
                     "New horizontal alignment, anchored at the bottom")},
           {"diameter", numberProp("New hole diameter in mm")}},
          {"id"}),
      false,
      [&session, modifiable](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const BoardItem item = findItem(board, requiredString(args, "id"));
        const QStringList fields = modifiable.value(item.type);
        for (auto it = args.begin(); it != args.end(); ++it) {
          if ((it.key() != "board") && (it.key() != "id") &&
              (!fields.contains(it.key()))) {
            throw RuntimeError(
                __FILE__, __LINE__,
                QString("The field '%1' does not apply to a %2. Valid: %3.")
                    .arg(it.key(), item.type, fields.join(", ")));
          }
        }
        auto length = [&args](const QString& key) {
          return PositiveLength(lengthMm(args.value(key), key));  // can throw
        };
        auto position = [&args]() {
          return pointMm(args.value("position"), "position");
        };
        if (item.trace) {
          auto cmd = std::make_unique<editor::CmdBoardNetLineEdit>(*item.trace);
          if (args.contains("width")) cmd->setWidth(length("width"));
          if (args.contains("layer")) {
            cmd->setLayer(copperLayer(args.value("layer"), board));
          }
          session.execCmd(cmd.release());  // can throw
        } else if (item.via) {
          auto cmd = std::make_unique<editor::CmdBoardViaEdit>(*item.via);
          if (args.contains("position")) cmd->setPosition(position(), false);
          if (args.contains("drill") || args.contains("size")) {
            cmd->setDrillAndSize(
                args.contains("drill") ? length("drill")
                                       : item.via->getDrillDiameter(),
                args.contains("size") ? length("size") : item.via->getSize(),
                false);
          }
          session.execCmd(cmd.release());  // can throw
        } else if (item.point) {
          auto cmd =
              std::make_unique<editor::CmdBoardNetPointEdit>(*item.point);
          if (args.contains("position")) cmd->setPosition(position(), false);
          session.execCmd(cmd.release());  // can throw
        } else if (item.plane) {
          auto cmd = std::make_unique<editor::CmdBoardPlaneEdit>(*item.plane);
          if (args.contains("vertices")) {
            cmd->setOutline(closedPathMm(args.value("vertices"), "vertices"),
                            false);
          }
          if (args.contains("layer")) {
            cmd->setLayer(copperLayer(args.value("layer"), board), false);
          }
          if (args.contains("net")) {
            NetSignal* net =
                session.getProject().getCircuit().getNetSignalByName(
                    requiredString(args, "net"));
            if (!net) {
              throw RuntimeError(__FILE__, __LINE__, "Net not found.");
            }
            cmd->setNetSignal(net);
          }
          if (args.contains("priority")) {
            cmd->setPriority(args.value("priority").toInt());
          }
          if (args.contains("connectStyle")) {
            cmd->setConnectStyle(connectStyle(args.value("connectStyle")));
          }
          if (args.contains("clearance")) {
            cmd->setMinClearanceToCopper(
                UnsignedLength(lengthMm(args.value("clearance"), "clearance")));
          }
          session.execCmd(cmd.release());  // can throw
        } else if (item.keepout) {
          auto cmd = std::make_unique<editor::CmdBoardZoneEdit>(*item.keepout);
          if (args.contains("vertices")) {
            cmd->setOutline(closedPathMm(args.value("vertices"), "vertices"),
                            false);
          }
          if (args.contains("layers")) {
            cmd->setLayers(zoneLayers(args.value("layers"), board), false);
          }
          if (args.contains("rules")) {
            cmd->setRules(zoneRules(args.value("rules")), false);
          }
          session.execCmd(cmd.release());  // can throw
        } else if (item.polygon) {
          auto cmd =
              std::make_unique<editor::CmdBoardPolygonEdit>(*item.polygon);
          if (args.contains("vertices")) {
            cmd->setPath(closedPathMm(args.value("vertices"), "vertices"),
                         false);
          }
          session.execCmd(cmd.release());  // can throw
        } else if (item.text) {
          auto cmd =
              std::make_unique<editor::CmdBoardStrokeTextEdit>(*item.text);
          if (args.contains("text")) {
            cmd->setText(requiredString(args, "text"), false);
          }
          if (args.contains("position")) cmd->setPosition(position(), false);
          if (args.contains("rotation")) {
            cmd->setRotation(angleDeg(args.value("rotation"), "rotation"),
                             false);
          }
          if (args.contains("height")) cmd->setHeight(length("height"), false);
          if (args.contains("align")) {
            cmd->setAlignment(
                Alignment(hAlign(args.value("align")), VAlign::bottom()),
                false);
          }
          if (args.contains("layer")) {
            const QJsonValue layerValue = args.value("layer");
            const Layer& layer = anyLayer(layerValue, board);
            cmd->setLayer(layer, false);
            cmd->setMirrored(layer.isBottom(), false);
          }
          session.execCmd(cmd.release());  // can throw
        } else if (item.hole) {
          auto cmd = std::make_unique<editor::CmdBoardHoleEdit>(*item.hole);
          if (args.contains("position")) {
            cmd->setPath(makeNonEmptyPath(position()), false);
          }
          if (args.contains("diameter")) {
            cmd->setDiameter(length("diameter"), false);
          }
          session.execCmd(cmd.release());  // can throw
        }
        return McpToolResult::text("Item changed.");
      },
  });

  server.addTool(McpTool{
      "board_get_settings",
      "Get the board setup: inner copper layer count, PCB thickness, design "
      "rules (defaults for new traces and vias), DRC settings (minimum "
      "values), net classes with their nets, and the available design rule "
      "presets of PCB manufacturers (for board_set_settings).",
      objectSchema({{"board", sBoard}}),
      true,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        QJsonObject drc;
        for (const DrcField& field : sDrcFields) {
          drc.insert(field.key, (board.getDrcSettings().*field.get)()->toMm());
        }
        QJsonArray sources;
        for (const DrcSettings::Source& source :
             board.getDrcSettings().getSources()) {
          sources.append(QString(*source.organizationName % ": " %
                                 *source.pcbDesignRulesName));
        }
        drc.insert("presets", sources);
        QJsonArray netClasses;
        for (const NetClass* nc :
             session.getProject().getCircuit().getNetClasses()) {
          QStringList nets;
          for (const NetSignal* net :
               session.getProject().getCircuit().getNetSignals()) {
            if (&net->getNetClass() == nc) nets.append(*net->getName());
          }
          Toolbox::sortNumeric(nets);
          netClasses.append(QJsonObject{
              {"name", *nc->getName()},
              {"defaultTraceWidth", optionalMm(nc->getDefaultTraceWidth())},
              {"defaultViaDrill", optionalMm(nc->getDefaultViaDrill())},
              {"minClearance", nc->getMinCopperCopperClearance()->toMm()},
              {"minWidth", nc->getMinCopperWidth()->toMm()},
              {"minViaDrill", nc->getMinViaDrillDiameter()->toMm()},
              {"nets", QJsonArray::fromStringList(nets)},
          });
        }
        QJsonArray presets{"default"};
        for (const auto& preset : drcPresets(session)) {
          presets.append(std::get<0>(preset));
        }
        return McpToolResult::json(QJsonObject{
            {"innerLayers", board.getInnerLayerCount()},
            {"thickness", board.getPcbThickness()->toMm()},
            {"designRules",
             QJsonObject{
                 {"defaultTraceWidth",
                  board.getDesignRules().getDefaultTraceWidth()->toMm()},
                 {"defaultViaDrill",
                  board.getDesignRules().getDefaultViaDrillDiameter()->toMm()},
             }},
            {"drc", drc},
            {"netClasses", netClasses},
            {"presets", presets},
        });
      },
  });

  QJsonObject settingsProperties{
      {"board", sBoard},
      {"preset",
       stringProp("Design rule preset of a PCB manufacturer as listed by "
                  "board_get_settings, or 'default'. Applied first, then "
                  "the other values.")},
      {"innerLayers",
       QJsonObject{{"type", "integer"},
                   {"minimum", 0},
                   {"maximum", Layer::innerCopperCount()},
                   {"description", "Number of inner copper layers"}}},
      {"thickness", numberProp("PCB thickness in mm")},
      {"defaultTraceWidth",
       numberProp("Default width of new traces in mm (if the net class "
                  "has none)")},
      {"defaultViaDrill",
       numberProp("Default drill of new vias in mm (if the net class has "
                  "none)")},
  };
  for (const DrcField& field : sDrcFields) {
    settingsProperties.insert(
        field.key, numberProp("DRC minimum in mm: " % field.description));
  }
  server.addTool(McpTool{
      "board_set_settings",
      "Change the board setup: apply a design rule preset of a PCB "
      "manufacturer, set inner copper layers (named 'inner1' ... in other "
      "tools), PCB thickness, defaults for new traces and vias, and DRC "
      "minimum values. Net classes are set with net_class_set.",
      objectSchema(settingsProperties),
      false,
      [&session](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        DrcSettings drc = board.getDrcSettings();
        BoardDesignRules rules = board.getDesignRules();
        if (args.contains("preset")) {
          const QString name = requiredString(args, "preset");
          QStringList names{"'default'"};
          bool found = (name.compare("default", Qt::CaseInsensitive) == 0);
          if (found) {
            drc = DrcSettings();
          }
          for (const auto& [presetName, orgDir, rulesUuid] :
               drcPresets(session)) {
            names.append("'" % presetName % "'");
            if ((!found) &&
                (presetName.compare(name, Qt::CaseInsensitive) == 0)) {
              std::unique_ptr<Organization> org =
                  openElement<Organization>(orgDir);
              const OrganizationPcbDesignRules* dru =
                  org->findPcbDesignRules(rulesUuid);
              if (!dru) throw LogicError(__FILE__, __LINE__);
              drc = dru->getDrcSettings(true);
              drc.setSources({DrcSettings::Source{
                  org->getUuid(), org->getNames().getDefaultValue(),
                  org->getVersion(), dru->getUuid(),
                  dru->getNames().getDefaultValue()}});
              found = true;
            }
          }
          if (!found) {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("Preset '%1' not found. Available: %2")
                                   .arg(name, names.join(", ")));
          }
          rules.adjustToDrcSettings(drc);
        }
        for (const DrcField& field : sDrcFields) {
          if (args.contains(field.key)) {
            (drc.*field.set)(
                UnsignedLength(lengthMm(args.value(field.key), field.key)));
          }
        }
        if (args.contains("defaultTraceWidth")) {
          rules.setDefaultTraceWidth(PositiveLength(
              lengthMm(args.value("defaultTraceWidth"), "defaultTraceWidth")));
        }
        if (args.contains("defaultViaDrill")) {
          rules.setDefaultViaDrillDiameter(PositiveLength(
              lengthMm(args.value("defaultViaDrill"), "defaultViaDrill")));
        }
        auto cmd = std::make_unique<editor::CmdBoardEdit>(board);
        cmd->setDesignRules(rules);
        cmd->setDrcSettings(drc);
        if (args.contains("innerLayers")) {
          cmd->setInnerLayerCount(args.value("innerLayers").toInt());
        }
        if (args.contains("thickness")) {
          cmd->setPcbThickness(
              PositiveLength(lengthMm(args.value("thickness"), "thickness")));
        }
        session.execCmd(cmd.release());  // can throw
        return McpToolResult::text("Board settings changed.");
      },
  });

  server.addTool(McpTool{
      "board_status",
      "Get the routing status of a board: unplaced parts, placed parts and "
      "the unrouted connections (air wires). Planes are filled first.",
      objectSchema({{"board", sBoard}}),
      true,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        Board& board = session.getBoard(args.value("board"));
        rebuildPlanes(board);  // can throw
        board.forceAirWiresRebuild();
        QJsonArray unplaced;
        QJsonArray placed;
        for (const ComponentInstance* cmp :
             project.getCircuit().getComponentInstances()) {
          if (cmp->getLibComponent().isSchematicOnly()) continue;
          if (const BI_Device* dev =
                  board.getDeviceInstanceByComponentUuid(cmp->getUuid())) {
            placed.append(QJsonObject{
                {"designator", *cmp->getName()},
                {"device", *dev->getLibDevice().getNames().getDefaultValue()},
                {"position", toJson(dev->getPosition())},
                {"rotation", dev->getRotation().toDeg()},
                {"side", dev->getMirrored() ? "bottom" : "top"},
            });
          } else {
            unplaced.append(*cmp->getName());
          }
        }
        QJsonArray airWires;
        for (const BI_AirWire* airWire : board.getAirWires()) {
          airWires.append(QJsonObject{
              {"net", *airWire->getNetSignal().getName()},
              {"from", anchorToJson(airWire->getP1())},
              {"to", anchorToJson(airWire->getP2())},
          });
        }
        QJsonObject result{
            {"unplacedParts", unplaced},
            {"placedParts", placed},
            {"unroutedConnections", airWires},
        };
        if (const auto bbox = board.calculateBoundingRect()) {
          result.insert("outline",
                        QJsonObject{{"from", toJson(bbox->first)},
                                    {"to", toJson(bbox->second)}});
        }
        return McpToolResult::json(result);
      },
  });

  server.addTool(McpTool{
      "output_jobs_run",
      "Generate the production data (Gerber/Excellon, assembly and "
      "schematic PDFs, pick&place, BOM, archives) into the project's "
      "output directory. Adds the default set of output jobs if the "
      "project has none yet (saved with project_save).",
      QJsonObject(),
      false,
      [&session](const QJsonObject&) {
        Project& project = session.getProject();
        if (project.getOutputJobs().isEmpty()) {
          auto gerber = GerberExcellonOutputJob::defaultStyle();
          auto archive = std::make_shared<ArchiveOutputJob>();
          archive->setInputJobs({{gerber->getUuid(), QString()}});
          OutputJobList defaults;
          defaults.append(GraphicsOutputJob::schematicPdf());
          defaults.append(GraphicsOutputJob::boardAssemblyPdf());
          defaults.append(gerber);
          defaults.append(std::make_shared<PickPlaceOutputJob>());
          defaults.append(std::make_shared<BomOutputJob>());
          defaults.append(archive);
          defaults.append(std::make_shared<LppzOutputJob>());
          auto cmd = std::make_unique<editor::CmdProjectEdit>(project);
          cmd->setOutputJobs(defaults);
          session.execCmd(cmd.release());  // can throw
        }
        OutputJobList& jobs = project.getOutputJobs();
        QStringList warnings;
        OutputJobRunner runner(project);
        QObject::connect(
            &runner, &OutputJobRunner::warning,
            [&warnings](const QString& msg) { warnings.append(msg); });
        runner.run(jobs.values());  // can throw
        QStringList files;
        for (const FilePath& fp : runner.getWrittenFiles()) {
          files.append(fp.toRelative(runner.getOutputDirectory()));
        }
        files.sort();
        return McpToolResult::json(QJsonObject{
            {"outputDirectory", runner.getOutputDirectory().toNative()},
            {"files", QJsonArray::fromStringList(files)},
            {"warnings", QJsonArray::fromStringList(warnings)},
        });
      },
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
