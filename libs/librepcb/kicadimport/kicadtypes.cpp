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
float deserialize(const SExpression& node) {
  bool ok = false;
  const float value = node.getValue().toFloat(&ok);
  if (ok) {
    return value;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid float: '%1'").arg(node.getValue()));
  }
}

template <>
double deserialize(const SExpression& node) {
  bool ok = false;
  const double value = node.getValue().toDouble(&ok);
  if (ok) {
    return value;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid double: '%1'").arg(node.getValue()));
  }
}

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
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid bool: '%1'").arg(value));
  }
}

static kicadimport::KiCadStrokeType deserializeStrokeType(
    const SExpression& node, MessageLogger& log) {
  if (node.getValue() == "default") {
    return kicadimport::KiCadStrokeType::Default;
  } else {
    log.warning(QString("Unknown stroke type '%1', using default instead.")
                    .arg(node.getValue()));
    return kicadimport::KiCadStrokeType::Unknown;
  }
}

static kicadimport::KiCadFillType deserializeFillType(const SExpression& node,
                                                      MessageLogger& log) {
  if (node.getValue() == "none") {
    return kicadimport::KiCadFillType::None;
  } else if (node.getValue() == "outline") {
    return kicadimport::KiCadFillType::Outline;
  } else if (node.getValue() == "background") {
    return kicadimport::KiCadFillType::Background;
  } else {
    log.warning(QString("Unknown fill type '%1', using default instead.")
                    .arg(node.getValue()));
    return kicadimport::KiCadFillType::Unknown;
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
    log.warning(QString("Unknown pin type '%1', using default instead.")
                    .arg(node.getValue()));
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
    log.warning(QString("Unknown pin shape '%1', using default instead.")
                    .arg(node.getValue()));
    return kicadimport::KiCadPinStyle::Unknown;
  }
}

/*******************************************************************************
 *  Struct KiCadProperty
 ******************************************************************************/

KiCadProperty KiCadProperty::parse(const SExpression& node,
                                   MessageLogger& log) {
  KiCadProperty obj;
  obj.key = node.getChild(0).getValue();
  obj.value = node.getChild(1).getValue();
  obj.position = deserialize<QVector3D>(node.getChild("at"));
  if (const SExpression* child = node.tryGetChild("layer/@0")) {
    obj.layer = child->getValue();
  }
  if (const SExpression* child = node.tryGetChild("effects/font/size")) {
    obj.fontSize = deserialize<QSizeF>(*child);
  }
  if (const SExpression* child =
          node.tryGetChild("effects/font/thickness/@0")) {
    obj.fontThickness = deserialize<qreal>(*child);
  } else {
    obj.fontThickness = 0;
  }
  if (const SExpression* child = node.tryGetChild("effects/hide")) {
    obj.hide = deserializeBool(*child);
  } else {
    obj.hide = false;
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadRectangle
 ******************************************************************************/

KiCadRectangle KiCadRectangle::parse(const SExpression& node,
                                     MessageLogger& log) {
  KiCadRectangle obj;
  obj.start = deserialize<QPointF>(node.getChild("start"));
  obj.end = deserialize<QPointF>(node.getChild("end"));
  obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  obj.strokeType = deserializeStrokeType(node.getChild("stroke/type/@0"), log);
  obj.fillType = deserializeFillType(node.getChild("fill/type/@0"), log);
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
  obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  obj.strokeType = deserializeStrokeType(node.getChild("stroke/type/@0"), log);
  obj.layer = node.getChild("layer").getValue();
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
  obj.strokeWidth = deserialize<qreal>(node.getChild("stroke/width/@0"));
  obj.strokeType = deserializeStrokeType(node.getChild("stroke/type/@0"), log);
  obj.layer = node.getChild("layer").getValue();
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
  obj.pos = deserialize<QVector3D>(node.getChild("at"));
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
  const QStringList nameParts = node.getChild(0).getValue().split("_");
  if (nameParts.count() < 3) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid symbol gate name: '%1'")
                           .arg(node.getChild(0).getValue()));
  }
  obj.name = node.getChild(0).getValue();
  bool ok;
  obj.index = nameParts.at(nameParts.count() - 2).toInt(&ok);
  if (!ok) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid symbol gate name: '%1'")
                           .arg(node.getChild(0).getValue()));
  }
  foreach (const SExpression* child, node.getChildren("rectangle")) {
    obj.rectangles.append(KiCadRectangle::parse(*child, log));
  }
  foreach (const SExpression* child, node.getChildren("pin")) {
    obj.pins.append(KiCadSymbolPin::parse(*child, log));
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadSymbol
 ******************************************************************************/

KiCadSymbol KiCadSymbol::parse(const SExpression& node, MessageLogger& log) {
  KiCadSymbol obj;
  obj.name = node.getChild(0).getValue();
  if (const SExpression* child = node.tryGetChild("exclude_from_sim/@0")) {
    obj.excludeFromSim = deserializeBool(*child);
  } else {
    obj.excludeFromSim = false;
  }
  if (const SExpression* child = node.tryGetChild("in_bom/@0")) {
    obj.inBom = deserializeBool(*child);
  } else {
    obj.inBom = true;
  }
  if (const SExpression* child = node.tryGetChild("on_board/@0")) {
    obj.onBoard = deserializeBool(*child);
  } else {
    obj.onBoard = true;
  }
  foreach (const SExpression* child, node.getChildren("property")) {
    obj.properties.append(KiCadProperty::parse(*child, log));
  }
  foreach (const SExpression* child, node.getChildren("symbol")) {
    obj.gates.append(KiCadSymbolGate::parse(*child, log));
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
    try {
      obj.symbols.append(KiCadSymbol::parse(*child, log));
    } catch (const Exception& e) {
      log.critical("Failed to parse symbol: " % e.getMsg());
    }
  }
  return obj;
}

/*******************************************************************************
 *  Struct KiCadFootprintModel
 ******************************************************************************/

KiCadFootprintModel KiCadFootprintModel::parse(const SExpression& node,
                                               MessageLogger& log) {
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
  obj.name = node.getChild(0).getValue();
  if (const SExpression* child = node.tryGetChild("version/@0")) {
    obj.version = deserialize<int>(*child);
  } else {
    obj.version = -1;
  }
  if (const SExpression* child = node.tryGetChild("generator/@0")) {
    obj.generator = child->getValue();
  }
  obj.isSmd =
      node.getChild("attr").containsChild(*SExpression::createToken("smd"));
  obj.isThroughHole = node.getChild("attr").containsChild(
      *SExpression::createToken("through_hole"));
  obj.boardOnly = node.getChild("attr").containsChild(
      *SExpression::createToken("board_only"));
  obj.excludeFromPosFiles = node.getChild("attr").containsChild(
      *SExpression::createToken("exclude_from_pos_files"));
  obj.excludeFromBom = node.getChild("attr").containsChild(
      *SExpression::createToken("exclude_from_bom"));
  foreach (const SExpression* child, node.getChildren("property")) {
    obj.properties.append(KiCadProperty::parse(*child, log));
  }
  foreach (const SExpression* child, node.getChildren("fp_line")) {
    obj.lines.append(KiCadFootprintLine::parse(*child, log));
  }
  foreach (const SExpression* child, node.getChildren("fp_arc")) {
    obj.arcs.append(KiCadFootprintArc::parse(*child, log));
  }
  foreach (const SExpression* child, node.getChildren("model")) {
    obj.models.append(KiCadFootprintModel::parse(*child, log));
  }
  return obj;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
