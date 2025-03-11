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
#include "kicadtypes.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/utils/messagelogger.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

template <>
QPointF deserialize(const SExpression& node) {
  return QPointF(deserialize<qreal>(node.getChild("@0")),
                 deserialize<qreal>(node.getChild("@1")));
}

template <>
QSizeF deserialize(const SExpression& node) {
  return QSizeF(deserialize<qreal>(node.getChild("@0")),
                deserialize<qreal>(node.getChild("@1")));
}

template <>
QVector3D deserialize(const SExpression& node) {
  return QVector3D(deserialize<float>(node.getChild("@0")),
                   deserialize<float>(node.getChild("@1")),
                   deserialize<float>(node.getChild("@2")));
}

namespace kicadimport {

static bool deserializeBool(const SExpression& node) {
  const QString value = node.getValue();
  if (value == "yes") {
    return true;
  } else if (value == "no") {
    return false;
  } else {
    throw RuntimeError(__FILE__, __LINE__, "Invalid bool: " + value);
  }
}

static Qt::Alignment deserializeAlignment(const SExpression& node) {
  Qt::Alignment align;
  if (node.containsChild(*SExpression::createToken("left"))) {
    align |= Qt::AlignLeft;
  } else if (node.containsChild(*SExpression::createToken("right"))) {
    align |= Qt::AlignRight;
  } else {
    align |= Qt::AlignHCenter;
  }
  if (node.containsChild(*SExpression::createToken("top"))) {
    align |= Qt::AlignTop;
  } else if (node.containsChild(*SExpression::createToken("bottom"))) {
    align |= Qt::AlignBottom;
  } else {
    align |= Qt::AlignVCenter;
  }
  return align;
}

static kicadimport::KiCadEdge deserializeEdge(const SExpression& node,
                                              MessageLogger& log) {
  if (node.getValue() == "top_left") {
    return kicadimport::KiCadEdge::TopLeft;
  } else if (node.getValue() == "top_right") {
    return kicadimport::KiCadEdge::TopRight;
  } else if (node.getValue() == "bottom_left") {
    return kicadimport::KiCadEdge::BottomLeft;
  } else if (node.getValue() == "bottom_right") {
    return kicadimport::KiCadEdge::BottomRight;
  } else {
    log.warning("Unknown edge: " + node.getValue());
    return kicadimport::KiCadEdge::Unknown;
  }
}

static kicadimport::KiCadStrokeType deserializeStrokeType(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "dash") {
    return kicadimport::KiCadStrokeType::Dash;
  } else if (node.getValue() == "dash_dot") {
    return kicadimport::KiCadStrokeType::DashDot;
  } else if (node.getValue() == "dash_dot_dot") {
    return kicadimport::KiCadStrokeType::DashDotDOt;
  } else if (node.getValue() == "dot") {
    return kicadimport::KiCadStrokeType::Dot;
  } else if (node.getValue() == "default") {
    return kicadimport::KiCadStrokeType::Default;
  } else if (node.getValue() == "solid") {
    return kicadimport::KiCadStrokeType::Solid;
  } else {
    log.warning("Unknown stroke type: " + node.getValue());
    return kicadimport::KiCadStrokeType::Unknown;
  }
}

static kicadimport::KiCadSymbolTextType deserializeTextType(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "reference") {
    return kicadimport::KiCadSymbolTextType::Reference;
  } else if (node.getValue() == "value") {
    return kicadimport::KiCadSymbolTextType::Value;
  } else if (node.getValue() == "user") {
    return kicadimport::KiCadSymbolTextType::User;
  } else {
    log.warning("Unknown text type: " + node.getValue());
    return kicadimport::KiCadSymbolTextType::Unknown;
  }
}

static kicadimport::KiCadSymbolFillType deserializeSymbolFillType(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "none") {
    return kicadimport::KiCadSymbolFillType::None;
  } else if (node.getValue() == "outline") {
    return kicadimport::KiCadSymbolFillType::Outline;
  } else if (node.getValue() == "background") {
    return kicadimport::KiCadSymbolFillType::Background;
  } else {
    log.warning("Unknown symbol fill type: " + node.getValue());
    return kicadimport::KiCadSymbolFillType::Unknown;
  }
}

static kicadimport::KiCadPinType deserializePinType(const SExpression& node,
                                                    MessageLogger& log) {
  if (node.getValue() == "input") {
    return kicadimport::KiCadPinType::Input;
  } else if (node.getValue() == "output") {
    return kicadimport::KiCadPinType::Output;
  } else if (node.getValue() == "bidirectional") {
    return kicadimport::KiCadPinType::Bidirectional;
  } else if (node.getValue() == "tri_state") {
    return kicadimport::KiCadPinType::TriState;
  } else if (node.getValue() == "passive") {
    return kicadimport::KiCadPinType::Passive;
  } else if (node.getValue() == "free") {
    return kicadimport::KiCadPinType::Free;
  } else if (node.getValue() == "unspecified") {
    return kicadimport::KiCadPinType::Unspecified;
  } else if (node.getValue() == "power_in") {
    return kicadimport::KiCadPinType::PowerIn;
  } else if (node.getValue() == "power_out") {
    return kicadimport::KiCadPinType::PowerOut;
  } else if (node.getValue() == "open_collector") {
    return kicadimport::KiCadPinType::OpenCollector;
  } else if (node.getValue() == "open_emitter") {
    return kicadimport::KiCadPinType::OpenEmitter;
  } else if (node.getValue() == "no_connect") {
    return kicadimport::KiCadPinType::NoConnect;
  } else {
    log.warning("Unknown pin type: " + node.getValue());
    return kicadimport::KiCadPinType::Unknown;
  }
}

static kicadimport::KiCadPinStyle deserializePinStyle(const SExpression& node,
                                                      MessageLogger& log) {
  if (node.getValue() == "line") {
    return kicadimport::KiCadPinStyle::Line;
  } else if (node.getValue() == "inverted") {
    return kicadimport::KiCadPinStyle::Inverted;
  } else if (node.getValue() == "clock") {
    return kicadimport::KiCadPinStyle::Clock;
  } else if (node.getValue() == "inverted_clock") {
    return kicadimport::KiCadPinStyle::InvertedClock;
  } else if (node.getValue() == "input_low") {
    return kicadimport::KiCadPinStyle::InputLow;
  } else if (node.getValue() == "clock_low") {
    return kicadimport::KiCadPinStyle::ClockLow;
  } else if (node.getValue() == "output_low") {
    return kicadimport::KiCadPinStyle::OutputLow;
  } else if (node.getValue() == "edge_clock_high") {
    return kicadimport::KiCadPinStyle::EdgeClockHigh;
  } else if (node.getValue() == "non_logic") {
    return kicadimport::KiCadPinStyle::NonLogic;
  } else {
    log.warning("Unknown pin shape: " + node.getValue());
    return kicadimport::KiCadPinStyle::Unknown;
  }
}

static kicadimport::KiCadFootprintFillType deserializeFootprintFillType(
    const SExpression& node, MessageLogger& log) {
  // Just yes/no since KiCad v9.
  if ((node.getValue() == "no") || (node.getValue() == "none")) {
    return kicadimport::KiCadFootprintFillType::None;
  } else if ((node.getValue() == "yes") || (node.getValue() == "solid")) {
    return kicadimport::KiCadFootprintFillType::Solid;
  } else {
    log.warning("Unknown footprint fill type: " + node.getValue());
    return kicadimport::KiCadFootprintFillType::Unknown;
  }
}

static kicadimport::KiCadPadType deserializePadType(const SExpression& node,
                                                    MessageLogger& log) {
  if (node.getValue() == "thru_hole") {
    return kicadimport::KiCadPadType::ThruHole;
  } else if (node.getValue() == "smd") {
    return kicadimport::KiCadPadType::Smd;
  } else if (node.getValue() == "connect") {
    return kicadimport::KiCadPadType::Connect;
  } else if (node.getValue() == "np_thru_hole") {
    return kicadimport::KiCadPadType::NpThruHole;
  } else {
    log.warning("Unknown pad type: " + node.getValue());
    return kicadimport::KiCadPadType::Unknown;
  }
}

static kicadimport::KiCadPadShape deserializePadShape(const SExpression& node,
                                                      MessageLogger& log) {
  if (node.getValue() == "circle") {
    return kicadimport::KiCadPadShape::Circle;
  } else if (node.getValue() == "rect") {
    return kicadimport::KiCadPadShape::Rect;
  } else if (node.getValue() == "oval") {
    return kicadimport::KiCadPadShape::Oval;
  } else if (node.getValue() == "trapezoid") {
    return kicadimport::KiCadPadShape::Trapezoid;
  } else if (node.getValue() == "roundrect") {
    return kicadimport::KiCadPadShape::RoundRect;
  } else if (node.getValue() == "custom") {
    return kicadimport::KiCadPadShape::Custom;
  } else {
    log.warning("Unknown pad shape: " + node.getValue());
    return kicadimport::KiCadPadShape::Unknown;
  }
}

static kicadimport::KiCadCustomPadAnchor deserializeCustomPadAnchor(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "circle") {
    return kicadimport::KiCadCustomPadAnchor::Circle;
  } else if (node.getValue() == "rect") {
    return kicadimport::KiCadCustomPadAnchor::Rect;
  } else {
    log.warning("Unknown custom pad anchor: " + node.getValue());
    return kicadimport::KiCadCustomPadAnchor::Unknown;
  }
}

static kicadimport::KiCadPadProperty deserializePadProperty(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "pad_prop_bga") {
    return kicadimport::KiCadPadProperty::Bga;
  } else if (node.getValue() == "pad_prop_fiducial_glob") {
    return kicadimport::KiCadPadProperty::FiducialGlobal;
  } else if (node.getValue() == "pad_prop_fiducial_loc") {
    return kicadimport::KiCadPadProperty::FiducialLocal;
  } else if (node.getValue() == "pad_prop_testpoint") {
    return kicadimport::KiCadPadProperty::Testpoint;
  } else if (node.getValue() == "pad_prop_heatsink") {
    return kicadimport::KiCadPadProperty::Heatsink;
  } else if (node.getValue() == "pad_prop_castellated") {
    return kicadimport::KiCadPadProperty::Castellated;
  } else {
    log.warning("Unknown pad property: " + node.getValue());
    return kicadimport::KiCadPadProperty::Unknown;
  }
}

static kicadimport::KiCadZoneConnect deserializeZoneConnect(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "0") {
    return kicadimport::KiCadZoneConnect::NoConnect;
  } else if (node.getValue() == "1") {
    return kicadimport::KiCadZoneConnect::ThermalReliefs;
  } else if (node.getValue() == "2") {
    return kicadimport::KiCadZoneConnect::Solid;
  } else {
    log.warning("Unknown zone connect: " + node.getValue());
    return kicadimport::KiCadZoneConnect::Unknown;
  }
}

static kicadimport::KiCadLayer deserializeLayer(const SExpression& node,
                                                MessageLogger& log) {
  if (node.getValue() == "*.Cu") {
    return kicadimport::KiCadLayer::AllCopper;
  } else if (node.getValue() == "*.Mask") {
    return kicadimport::KiCadLayer::AllSolderMask;
  } else if (node.getValue() == "*.SilkS") {
    return kicadimport::KiCadLayer::AllSilkscreen;
  } else if (node.getValue() == "F&B.Cu") {
    return kicadimport::KiCadLayer::FrontAndBackCopper;
  } else if (node.getValue() == "F.Adhes") {
    return kicadimport::KiCadLayer::FrontAdhesion;
  } else if (node.getValue() == "F.Cu") {
    return kicadimport::KiCadLayer::FrontCopper;
  } else if (node.getValue() == "F.CrtYd") {
    return kicadimport::KiCadLayer::FrontCourtyard;
  } else if (node.getValue() == "F.Fab") {
    return kicadimport::KiCadLayer::FrontFabrication;
  } else if (node.getValue() == "F.Paste") {
    return kicadimport::KiCadLayer::FrontPaste;
  } else if (node.getValue() == "F.SilkS") {
    return kicadimport::KiCadLayer::FrontSilkscreen;
  } else if (node.getValue() == "F.Mask") {
    return kicadimport::KiCadLayer::FrontSolderMask;
  } else if (node.getValue().startsWith("In") &&
             node.getValue().endsWith(".Cu")) {
    QString numStr = node.getValue();
    numStr.replace("In", "");
    numStr.replace(".Cu", "");
    bool ok = false;
    const int num = numStr.toInt(&ok);
    if ((ok) && (num >= 1) && (num <= 30)) {
      return static_cast<KiCadLayer>(
          static_cast<int>(kicadimport::KiCadLayer::InnerCopper1) + num - 1);
    } else {
      log.warning("Unknown layer: " + node.getValue());
      return kicadimport::KiCadLayer::Unknown;
    }
  } else if (node.getValue() == "B.Adhes") {
    return kicadimport::KiCadLayer::BackAdhesion;
  } else if (node.getValue() == "B.Cu") {
    return kicadimport::KiCadLayer::BackCopper;
  } else if (node.getValue() == "B.CrtYd") {
    return kicadimport::KiCadLayer::BackCourtyard;
  } else if (node.getValue() == "B.Fab") {
    return kicadimport::KiCadLayer::BackFabrication;
  } else if (node.getValue() == "B.Paste") {
    return kicadimport::KiCadLayer::BackPaste;
  } else if (node.getValue() == "B.SilkS") {
    return kicadimport::KiCadLayer::BackSilkscreen;
  } else if (node.getValue() == "B.Mask") {
    return kicadimport::KiCadLayer::BackSolderMask;
  } else if (node.getValue() == "Edge.Cuts") {
    return kicadimport::KiCadLayer::BoardOutline;
  } else if (node.getValue() == "Cmts.User") {
    return kicadimport::KiCadLayer::UserComment;
  } else if (node.getValue() == "Dwgs.User") {
    return kicadimport::KiCadLayer::UserDrawing;
  } else if (node.getValue() == "User.1") {
    return kicadimport::KiCadLayer::User1;
  } else if (node.getValue() == "User.2") {
    return kicadimport::KiCadLayer::User2;
  } else if (node.getValue() == "User.3") {
    return kicadimport::KiCadLayer::User3;
  } else if (node.getValue() == "User.4") {
    return kicadimport::KiCadLayer::User4;
  } else if (node.getValue() == "User.5") {
    return kicadimport::KiCadLayer::User5;
  } else if (node.getValue() == "User.6") {
    return kicadimport::KiCadLayer::User6;
  } else if (node.getValue() == "User.7") {
    return kicadimport::KiCadLayer::User7;
  } else if (node.getValue() == "User.8") {
    return kicadimport::KiCadLayer::User8;
  } else if (node.getValue() == "User.9") {
    return kicadimport::KiCadLayer::User9;
  } else {
    log.warning("Unknown layer: " + node.getValue());
    return kicadimport::KiCadLayer::Unknown;
  }
}

/*******************************************************************************
 *  Struct KiCadProperty
 ******************************************************************************/

KiCadProperty KiCadProperty::parse(const SExpression& node,
                                   MessageLogger& log) {
  Q_UNUSED(log);

  KiCadProperty obj;
  obj.key = node.getChild(0).getValue();
  obj.value = node.getChild(1).getValue();
  obj.position = deserialize<QPointF>(node.getChild("at"));
  if (const SExpression* child = node.tryGetChild("at/@2")) {
    if (child->getValue() == "unlocked") {
      obj.unlocked = true;  // KiCad v6 compatibility.
    } else {
      obj.rotation = deserialize<qreal>(*child);
    }
  }
  if (const SExpression* child = node.tryGetChild("layer/@0")) {
    obj.layer = child->getValue();
  }
  if (const SExpression* child = node.tryGetChild("effects/font/size")) {
    obj.fontSize = deserialize<QSizeF>(*child);
  }
  if (const SExpression* child =
          node.tryGetChild("effects/font/thickness/@0")) {
    obj.fontThickness = deserialize<qreal>(*child);
  }
  if (const SExpression* child = node.tryGetChild("effects/justify")) {
    obj.alignment = deserializeAlignment(*child);
    obj.mirror = child->containsChild(*SExpression::createToken("mirror"));
  }
  if (const SExpression* child = node.tryGetChild("unlocked/@0")) {
    obj.unlocked = (child->getValue() == "yes");
  }
  if (const SExpression* child = node.tryGetChild("effects/hide/@0")) {
    obj.hide = deserializeBool(*child);
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadGraphicalLine
 ******************************************************************************/

KiCadGraphicalLine KiCadGraphicalLine::parse(const SExpression& node,
                                             MessageLogger& log) {
  Q_UNUSED(log);

  KiCadGraphicalLine obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  obj.width = deserialize<qreal>(node.getChild("width/@0"));
  return obj;
}

/*******************************************************************************
 *  Struct KiCadGraphicalArc
 ******************************************************************************/

KiCadGraphicalArc KiCadGraphicalArc::parse(const SExpression& node,
                                           MessageLogger& log) {
  Q_UNUSED(log);

  KiCadGraphicalArc obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.mid = deserialize<QPointF>(node.getChild("mid"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  obj.width = deserialize<qreal>(node.getChild("width/@0"));
  return obj;
}

/*******************************************************************************
 *  Struct KiCadGraphicalCircle
 ******************************************************************************/

KiCadGraphicalCircle KiCadGraphicalCircle::parse(const SExpression& node,
                                                 MessageLogger& log) {
  Q_UNUSED(log);

  KiCadGraphicalCircle obj;
  obj.center = deserialize<QPointF>(node.getChild("center"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  obj.width = deserialize<qreal>(node.getChild("width/@0"));
  if (const SExpression* child = node.tryGetChild("fill/@0")) {
    obj.fill = (child->getValue() == "yes") || (child->getValue() == "solid");
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadGraphicalPolygon
 ******************************************************************************/

KiCadGraphicalPolygon KiCadGraphicalPolygon::parse(const SExpression& node,
                                                   MessageLogger& log) {
  Q_UNUSED(log);

  KiCadGraphicalPolygon obj;
  foreach (const SExpression* child, node.getChild("pts").getChildren("xy")) {
    obj.coordinates.append(deserialize<QPointF>(*child));
  }
  obj.width = deserialize<qreal>(node.getChild("width/@0"));
  if (const SExpression* child = node.tryGetChild("fill/@0")) {
    obj.fill = (child->getValue() == "yes") || (child->getValue() == "solid");
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadZone
 ******************************************************************************/

KiCadZone KiCadZone::parse(const SExpression& node, MessageLogger& log) {
  auto parseKeepOutBool = [&node, &log](const QString& name) {
    const QString value = node.getChild("keepout/" % name % "/@0").getValue();
    if (value == "allowed") {
      return false;
    } else if (value == "not_allowed") {
      return true;
    } else {
      log.warning("Unknown keepout value: " + value);
      return false;
    }
  };

  KiCadZone obj;
  if (const SExpression* child = node.tryGetChild("layer/@0")) {
    obj.layers.append(deserializeLayer(*child, log));
  } else {
    foreach (const SExpression* child,
             node.getChild("layers").getChildren(SExpression::Type::String)) {
      obj.layers.append(deserializeLayer(*child, log));
    }
  }
  obj.keepOutTracks = parseKeepOutBool("tracks");
  obj.keepOutVias = parseKeepOutBool("vias");
  obj.keepOutPads = parseKeepOutBool("pads");
  obj.keepOutCopperPour = parseKeepOutBool("copperpour");
  obj.keepOutFootprints = parseKeepOutBool("footprints");
  foreach (const SExpression* child,
           node.getChild("polygon/pts").getChildren("xy")) {
    obj.polygon.append(deserialize<QPointF>(*child));
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolArc
 ******************************************************************************/

KiCadSymbolArc KiCadSymbolArc::parse(const SExpression& node,
                                     MessageLogger& log) {
  KiCadSymbolArc obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.mid = deserialize<QPointF>(node.getChild("mid"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  obj.fillType = deserializeSymbolFillType(node.getChild("fill/type/@0"), log);
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolCircle
 ******************************************************************************/

KiCadSymbolCircle KiCadSymbolCircle::parse(const SExpression& node,
                                           MessageLogger& log) {
  KiCadSymbolCircle obj;
  obj.center = deserialize<QPointF>(node.getChild("center"));
  obj.radius = deserialize<qreal>(node.getChild("radius/@0"));
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  obj.fillType = deserializeSymbolFillType(node.getChild("fill/type/@0"), log);
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolRectangle
 ******************************************************************************/

KiCadSymbolRectangle KiCadSymbolRectangle::parse(const SExpression& node,
                                                 MessageLogger& log) {
  KiCadSymbolRectangle obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  obj.fillType = deserializeSymbolFillType(node.getChild("fill/type/@0"), log);
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolPolyline
 ******************************************************************************/

KiCadSymbolPolyline KiCadSymbolPolyline::parse(const SExpression& node,
                                               MessageLogger& log) {
  KiCadSymbolPolyline obj;
  foreach (const SExpression* child, node.getChild("pts").getChildren("xy")) {
    obj.coordinates.append(deserialize<QPointF>(*child));
  }
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  obj.fillType = deserializeSymbolFillType(node.getChild("fill/type/@0"), log);
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolText
 ******************************************************************************/

KiCadSymbolText KiCadSymbolText::parse(const SExpression& node,
                                       MessageLogger& log) {
  Q_UNUSED(log);

  KiCadSymbolText obj;
  obj.text = node.getChild("@0").getValue();
  obj.position = deserialize<QPointF>(node.getChild("at"));
  if (const SExpression* child = node.tryGetChild("at/@2")) {
    obj.rotation = deserialize<qreal>(*child);
  }
  if (const SExpression* child = node.tryGetChild("effects/font/size")) {
    obj.fontSize = deserialize<QSizeF>(*child);
  }
  if (const SExpression* child =
          node.tryGetChild("effects/font/thickness/@0")) {
    obj.fontThickness = deserialize<qreal>(*child);
  }
  if (const SExpression* child = node.tryGetChild("effects/justify")) {
    obj.alignment = deserializeAlignment(*child);
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolPin
 ******************************************************************************/

KiCadSymbolPin KiCadSymbolPin::parse(const SExpression& node,
                                     MessageLogger& log) {
  KiCadSymbolPin obj;
  obj.type = deserializePinType(node.getChild(0), log);
  obj.shape = deserializePinStyle(node.getChild(1), log);
  obj.position = deserialize<QPointF>(node.getChild("at"));
  if (const SExpression* child = node.tryGetChild("at/@2")) {
    obj.rotation = deserialize<qreal>(*child);
  }
  obj.length = deserialize<qreal>(node.getChild("length/@0"));
  obj.name = node.getChild("name/@0").getValue();
  obj.number = node.getChild("number/@0").getValue();
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolGate
 ******************************************************************************/

KiCadSymbolGate KiCadSymbolGate::parse(const SExpression& node,
                                       MessageLogger& log) {
  KiCadSymbolGate obj;
  obj.name = node.getChild(0).getValue();
  const QStringList nameParts = obj.name.split("_");
  bool ok1, ok2;
  obj.index = nameParts.value(nameParts.count() - 2).toInt(&ok1);
  const int style = nameParts.value(nameParts.count() - 1).toInt(&ok2);
  if ((nameParts.count() < 3) || (!ok1) || (!ok2)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Invalid symbol gate name: " + obj.name);
  }
  if (style == 0) {
    obj.style = Style::Common;
  } else if (style == 1) {
    obj.style = Style::Base;
  } else if (style == 2) {
    obj.style = Style::DeMorgan;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown symbol gate style %1.").arg(style));
  }
  foreach (const SExpression* child,
           node.getChildren(SExpression::Type::List)) {
    if (child->getName() == "arc") {
      obj.arcs.append(KiCadSymbolArc::parse(*child, log));
    } else if (child->getName() == "circle") {
      obj.circles.append(KiCadSymbolCircle::parse(*child, log));
    } else if (child->getName() == "rectangle") {
      obj.rectangles.append(KiCadSymbolRectangle::parse(*child, log));
    } else if (child->getName() == "polyline") {
      obj.polylines.append(KiCadSymbolPolyline::parse(*child, log));
    } else if (child->getName() == "text") {
      obj.texts.append(KiCadSymbolText::parse(*child, log));
    } else if (child->getName() == "pin") {
      obj.pins.append(KiCadSymbolPin::parse(*child, log));
    } else {
      log.warning(
          tr("Unsupported symbol gate child: '%1'").arg(child->getName()));
    }
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbol
 ******************************************************************************/

KiCadSymbol KiCadSymbol::parse(const SExpression& node, MessageLogger& log) {
  KiCadSymbol obj;
  obj.name = node.getChild("@0").getValue();
  foreach (const SExpression* child,
           node.getChildren(SExpression::Type::List)) {
    if (child->getName() == "extends") {
      obj.extends = child->getChild("@0").getValue();
    } else if (child->getName() == "pin_names") {
      obj.hidePinNames =
          child->containsChild(*SExpression::createToken("hide"));
      if (const SExpression* offset = child->tryGetChild("offset/@0")) {
        obj.pinNamesOffset = deserialize<qreal>(*offset);
      }
    } else if (child->getName() == "pin_numbers") {
      obj.hidePinNumbers =
          child->containsChild(*SExpression::createToken("hide"));
    } else if (child->getName() == "exclude_from_sim") {
      obj.excludeFromSim = deserializeBool(child->getChild("@0"));
    } else if (child->getName() == "in_bom") {
      obj.inBom = deserializeBool(child->getChild("@0"));
    } else if (child->getName() == "on_board") {
      obj.onBoard = deserializeBool(child->getChild("@0"));
    } else if (child->getName() == "property") {
      obj.properties.append(KiCadProperty::parse(*child, log));
    } else if (child->getName() == "symbol") {
      obj.gates.append(KiCadSymbolGate::parse(*child, log));
    } else if (child->getName() == "embedded_fonts") {
      // New in KiCad v9, ignoring for now.
    } else {
      log.warning(tr("Unsupported symbol child: '%1'").arg(child->getName()));
    }
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbolLibrary
 ******************************************************************************/

KiCadSymbolLibrary KiCadSymbolLibrary::parse(const SExpression& node,
                                             MessageLogger& log) {
  if (node.getName() != "kicad_symbol_lib") {
    throw RuntimeError(__FILE__, __LINE__,
                       "File does not seem to be a KiCad symbol library.");
  }

  KiCadSymbolLibrary obj;
  obj.version = deserialize<int>(node.getChild("version/@0"));
  obj.generator = node.getChild("generator/@0").getValue();
  foreach (const SExpression* child, node.getChildren("symbol")) {
    QString symbolName;
    try {
      symbolName = child->getChild("@0").getValue();
      obj.symbols.append(KiCadSymbol::parse(*child, log));
    } catch (const Exception& e) {
      log.critical(QString("Failed to parse symbol '%1': %2")
                       .arg(symbolName)
                       .arg(e.getMsg()));
    }
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintLine
 ******************************************************************************/

KiCadFootprintLine KiCadFootprintLine::parse(const SExpression& node,
                                             MessageLogger& log) {
  KiCadFootprintLine obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  obj.layer = deserializeLayer(node.getChild("layer/@0"), log);
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintArc
 ******************************************************************************/

KiCadFootprintArc KiCadFootprintArc::parse(const SExpression& node,
                                           MessageLogger& log) {
  KiCadFootprintArc obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.mid = deserialize<QPointF>(node.getChild("mid"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  obj.layer = deserializeLayer(node.getChild("layer/@0"), log);
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintCircle
 ******************************************************************************/

KiCadFootprintCircle KiCadFootprintCircle::parse(const SExpression& node,
                                                 MessageLogger& log) {
  KiCadFootprintCircle obj;
  obj.center = deserialize<QPointF>(node.getChild("center"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  obj.layer = deserializeLayer(node.getChild("layer/@0"), log);
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  if (const SExpression* child = node.tryGetChild("fill/@0")) {
    obj.fillType = deserializeFootprintFillType(*child, log);
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintRectangle
 ******************************************************************************/

KiCadFootprintRectangle KiCadFootprintRectangle::parse(const SExpression& node,
                                                       MessageLogger& log) {
  KiCadFootprintRectangle obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  obj.layer = deserializeLayer(node.getChild("layer/@0"), log);
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  if (const SExpression* child = node.tryGetChild("fill/@0")) {
    obj.fillType = deserializeFootprintFillType(*child, log);
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintPolygon
 ******************************************************************************/

KiCadFootprintPolygon KiCadFootprintPolygon::parse(const SExpression& node,
                                                   MessageLogger& log) {
  KiCadFootprintPolygon obj;
  foreach (const SExpression* child, node.getChild("pts").getChildren("xy")) {
    obj.coordinates.append(deserialize<QPointF>(*child));
  }
  obj.layer = deserializeLayer(node.getChild("layer/@0"), log);
  if (const SExpression* child = node.tryGetChild("width/@0")) {
    obj.strokeWidth = deserialize<qreal>(*child);  // KiCad v6 compatibility.
  } else {
    obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  }
  if (const SExpression* child = node.tryGetChild("stroke/type/@0")) {
    obj.strokeType = deserializeStrokeType(*child, log);
  }
  if (const SExpression* child = node.tryGetChild("fill/@0")) {
    obj.fillType = deserializeFootprintFillType(*child, log);
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintText
 ******************************************************************************/

KiCadFootprintText KiCadFootprintText::parse(const SExpression& node,
                                             MessageLogger& log) {
  KiCadFootprintText obj;
  obj.type = deserializeTextType(node.getChild("@0"), log);
  obj.text = node.getChild("@1").getValue();
  obj.position = deserialize<QPointF>(node.getChild("at"));
  if (const SExpression* child = node.tryGetChild("at/@2")) {
    if (child->getValue() == "unlocked") {  // KiCad v6 compatibility.
      obj.unlocked = true;
    } else {
      obj.rotation = deserialize<qreal>(*child);
    }
  }
  obj.layer = deserializeLayer(node.getChild("layer/@0"), log);
  if (const SExpression* child = node.tryGetChild("effects/font/size")) {
    obj.fontSize = deserialize<QSizeF>(*child);
  }
  if (const SExpression* child =
          node.tryGetChild("effects/font/thickness/@0")) {
    obj.fontThickness = deserialize<qreal>(*child);
  }
  if (const SExpression* child = node.tryGetChild("effects/justify")) {
    obj.alignment = deserializeAlignment(*child);
    obj.mirror = child->containsChild(*SExpression::createToken("mirror"));
  }
  if (const SExpression* child = node.tryGetChild("unlocked/@0")) {
    obj.unlocked = (child->getValue() == "yes");
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintPad
 ******************************************************************************/

KiCadFootprintPad KiCadFootprintPad::parse(const SExpression& node,
                                           MessageLogger& log) {
  KiCadFootprintPad obj;
  obj.number = node.getChild("@0").getValue();
  obj.type = deserializePadType(node.getChild("@1"), log);
  obj.shape = deserializePadShape(node.getChild("@2"), log);
  foreach (const SExpression* child,
           node.getChildren(SExpression::Type::List)) {
    if (child->getName() == "at") {
      obj.position = deserialize<QPointF>(*child);
      if (const SExpression* rot = child->tryGetChild("@2")) {
        obj.rotation = deserialize<qreal>(*rot);
      }
    } else if (child->getName() == "size") {
      obj.size = deserialize<QSizeF>(*child);
    } else if (child->getName() == "drill") {
      const SExpression* child0 = child->tryGetChild("@0");
      const SExpression* child1 = child->tryGetChild("@1");
      const SExpression* child2 = child->tryGetChild("@2");
      const bool isOval =
          child0 && (child0->isToken()) && (child0->getValue() == "oval");
      if (isOval) {
        child0 = child1;
        child1 = child2;
        child2 = nullptr;
      }
      if (child0 && (child0->isToken())) {
        obj.drill.setWidth(deserialize<qreal>(*child0));
        obj.drill.setHeight(deserialize<qreal>(*child0));
      }
      if (isOval && child1 && (child1->isToken())) {
        obj.drill.setHeight(deserialize<qreal>(*child1));
      }
      if (const SExpression* offset = child->tryGetChild("offset")) {
        obj.offset = deserialize<QPointF>(*offset);
      }
    } else if (child->getName() == "layers") {
      // KiCad v6 seems to had no quotes in some cases, thus we also need to
      // take tokens into account.
      foreach (const SExpression* layer,
               child->getChildren(SExpression::Type::String) +
                   child->getChildren(SExpression::Type::Token)) {
        obj.layers.append(deserializeLayer(*layer, log));
      }
    } else if (child->getName() == "property") {
      obj.property = deserializePadProperty(child->getChild("@0"), log);
    } else if (child->getName() == "solder_mask_margin") {
      obj.solderMaskMargin = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "solder_paste_margin") {
      obj.solderPasteMargin = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "solder_paste_margin_ratio") {
      obj.solderPasteMarginRatio = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "thermal_bridge_angle") {
      obj.thermalBridgeAngle = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "thermal_bridge_width") {
      obj.thermalBridgeWidth = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "zone_connect") {
      // Not supported yet.
    } else if (child->getName() == "die_length") {
      // Not supported yet.
    } else if (child->getName() == "clearance") {
      obj.clearance = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "remove_unused_layers") {
      obj.removeUnusedLayers = deserializeBool(child->getChild("@0"));
    } else if (child->getName() == "keep_end_layers") {
      // Not supported yet.
    } else if (child->getName() == "roundrect_rratio") {
      obj.roundRectRRatio = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "rect_delta") {
      obj.rectDelta = deserialize<QSizeF>(*child);
    } else if (child->getName() == "chamfer_ratio") {
      obj.chamferRatio = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "chamfer") {
      foreach (const SExpression* chamfer,
               child->getChildren(SExpression::Type::Token)) {
        obj.chamferEdges.append(deserializeEdge(*chamfer, log));
      }
    } else if (child->getName() == "options") {
      foreach (const SExpression* option,
               child->getChildren(SExpression::Type::List)) {
        if (option->getName() == "clearance") {
          const QString clr = option->getChild("@0").getValue();
          if (clr != "outline") {
            log.warning(tr("Unsupported pad clearance: '%1'").arg(clr));
          }
        } else if (option->getName() == "anchor") {
          obj.customPadAnchor =
              deserializeCustomPadAnchor(option->getChild("@0"), log);
        } else {
          log.warning(
              tr("Unsupported pad option: '%1'").arg(option->getName()));
        }
      }
    } else if (child->getName() == "primitives") {
      foreach (const SExpression* primitive,
               child->getChildren(SExpression::Type::List)) {
        if (primitive->getName() == "gr_line") {
          obj.graphicalLines.append(KiCadGraphicalLine::parse(*primitive, log));
        } else if (primitive->getName() == "gr_arc") {
          obj.graphicalArcs.append(KiCadGraphicalArc::parse(*primitive, log));
        } else if (primitive->getName() == "gr_circle") {
          obj.graphicalCircles.append(
              KiCadGraphicalCircle::parse(*primitive, log));
        } else if (primitive->getName() == "gr_poly") {
          obj.graphicalPolygons.append(
              KiCadGraphicalPolygon::parse(*primitive, log));
        } else {
          log.warning(
              tr("Unsupported pad primitive: '%1'").arg(primitive->getName()));
        }
      }
    } else if (child->getName() == "uuid") {
      // Ignored for now.
    } else if (child->getName() == "tstamp") {
      // Ignored for now.
    } else {
      log.warning(tr("Unsupported pad child: '%1'").arg(child->getName()));
    }
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintModel
 ******************************************************************************/

KiCadFootprintModel KiCadFootprintModel::parse(const SExpression& node,
                                               MessageLogger& log) {
  Q_UNUSED(log);

  KiCadFootprintModel obj;
  obj.path = node.getChild(0).getValue();
  if (const SExpression* child = node.tryGetChild("at/xyz")) {
    obj.offset = deserialize<QVector3D>(*child);
  } else {
    obj.offset = deserialize<QVector3D>(node.getChild("offset/xyz"));
  }
  obj.scale = deserialize<QVector3D>(node.getChild("scale/xyz"));
  obj.rotate = deserialize<QVector3D>(node.getChild("rotate/xyz"));
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprint
 ******************************************************************************/

KiCadFootprint KiCadFootprint::parse(const SExpression& node,
                                     MessageLogger& log) {
  if ((node.getName() != "footprint") && (node.getName() != "module")) {
    throw RuntimeError(__FILE__, __LINE__,
                       "File does not seem to be a KiCad footprint.");
  }

  KiCadFootprint obj;
  obj.name = node.getChild("@0").getValue();
  foreach (const SExpression* child,
           node.getChildren(SExpression::Type::List)) {
    if (child->getName() == "version") {
      obj.version = deserialize<int>(child->getChild("@0"));
    } else if (child->getName() == "generator") {
      obj.generator = child->getChild("@0").getValue();
    } else if (child->getName() == "generator_version") {
      // Ignore.
    } else if (child->getName() == "layer") {
      obj.layer = deserializeLayer(child->getChild("@0"), log);
    } else if (child->getName() == "descr") {
      obj.description = child->getChild("@0").getValue();
    } else if (child->getName() == "tags") {
      obj.tags = child->getChild("@0").getValue();
    } else if (child->getName() == "attr") {
      if (child->containsChild(*SExpression::createToken("smd"))) {
        obj.isSmd = true;
      }
      if (child->containsChild(*SExpression::createToken("through_hole"))) {
        obj.isThroughHole = true;
      }
      if (child->containsChild(*SExpression::createToken("board_only"))) {
        obj.boardOnly = true;
      }
      if (child->containsChild(
              *SExpression::createToken("exclude_from_pos_files"))) {
        obj.excludeFromPosFiles = true;
      }
      if (child->containsChild(*SExpression::createToken("exclude_from_bom"))) {
        obj.excludeFromBom = true;
      }
    } else if (child->getName() == "solder_mask_margin") {
      obj.solderMaskMargin = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "solder_paste_ratio") {
      obj.solderPasteRatio = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "solder_paste_margin") {
      obj.solderPasteMargin = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "clearance") {
      obj.clearance = deserialize<qreal>(child->getChild("@0"));
    } else if (child->getName() == "zone_connect") {
      obj.zoneConnect = deserializeZoneConnect(child->getChild("@0"), log);
    } else if (child->getName() == "net_tie_pad_groups") {
      foreach (const SExpression* grp,
               child->getChildren(SExpression::Type::String)) {
        QStringList pads;
        foreach (const QString& pad, grp->getValue().split(",")) {
          if (!pad.trimmed().isEmpty()) {
            pads.append(pad.trimmed());
          }
        }
        obj.netTiePadGroups.append(pads);
      }
    } else if (child->getName() == "property") {
      obj.properties.append(KiCadProperty::parse(*child, log));
    } else if (child->getName() == "fp_line") {
      obj.lines.append(KiCadFootprintLine::parse(*child, log));
    } else if (child->getName() == "fp_arc") {
      obj.arcs.append(KiCadFootprintArc::parse(*child, log));
    } else if (child->getName() == "fp_circle") {
      obj.circles.append(KiCadFootprintCircle::parse(*child, log));
    } else if (child->getName() == "fp_rect") {
      obj.rectangles.append(KiCadFootprintRectangle::parse(*child, log));
    } else if (child->getName() == "fp_poly") {
      obj.polygons.append(KiCadFootprintPolygon::parse(*child, log));
    } else if (child->getName() == "fp_text") {
      obj.texts.append(KiCadFootprintText::parse(*child, log));
    } else if (child->getName() == "zone") {
      obj.zones.append(KiCadZone::parse(*child, log));
    } else if (child->getName() == "pad") {
      obj.pads.append(KiCadFootprintPad::parse(*child, log));
    } else if (child->getName() == "group") {
      // Ignore.
    } else if (child->getName() == "embedded_fonts") {
      // New in KiCad v9, ignoring for now.
    } else if (child->getName() == "model") {
      obj.models.append(KiCadFootprintModel::parse(*child, log));
    } else {
      log.warning(
          tr("Unsupported footprint child: '%1'").arg(child->getName()));
    }
  }
  return obj;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
