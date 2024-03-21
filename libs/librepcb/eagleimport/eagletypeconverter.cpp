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
#include "eagletypeconverter.h"

#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/attribute/attributekey.h>
#include <librepcb/core/attribute/attrtypestring.h>
#include <librepcb/core/qtcompat.h>
#include <librepcb/core/types/boundedunsignedratio.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/utils/clipperhelpers.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/tangentpathjoiner.h>
#include <parseagle/board/param.h>
#include <parseagle/common/attribute.h>
#include <parseagle/common/circle.h>
#include <parseagle/common/frame.h>
#include <parseagle/common/grid.h>
#include <parseagle/common/point.h>
#include <parseagle/common/polygon.h>
#include <parseagle/common/rectangle.h>
#include <parseagle/common/rotation.h>
#include <parseagle/common/text.h>
#include <parseagle/common/wire.h>
#include <parseagle/package/hole.h>
#include <parseagle/package/smtpad.h>
#include <parseagle/package/thtpad.h>
#include <parseagle/symbol/pin.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ElementName EagleTypeConverter::convertElementName(const QString& n) {
  QString name = cleanElementName(n);
  if (name.isEmpty()) {
    name = "Unnamed";  // No tr() to ensure valid ElementName.
  }
  return ElementName(name);  // Can theoretically throw, but should not.
}

QString EagleTypeConverter::convertElementDescription(const QString& d) {
  QTextDocument doc;
  doc.setHtml(QString(d).replace("\n", "<br/>"));
  return doc.toPlainText()
      .trimmed()
      .split("\n", QtCompat::skipEmptyParts())
      .join("\n");
}

ElementName EagleTypeConverter::convertComponentName(QString n) {
  if ((n.length() > 1) && (n.endsWith("-") || n.endsWith("_"))) {
    n.chop(1);
  }
  return convertElementName(n);  // Can theoretically throw, but should not.
}

ElementName EagleTypeConverter::convertDeviceName(const QString& deviceSetName,
                                                  const QString& deviceName) {
  const bool addSeparator = (!deviceSetName.endsWith("-")) &&
      (!deviceSetName.endsWith("_")) && (!deviceName.startsWith("-")) &&
      (!deviceName.startsWith("_"));

  QString name = deviceSetName;
  if (addSeparator && (!deviceName.isEmpty())) {
    name += "-";
  }
  name += deviceName;
  return convertElementName(name);  // Can theoretically throw, but should not.
}

ComponentPrefix EagleTypeConverter::convertComponentPrefix(const QString& p) {
  return ComponentPrefix(
      cleanComponentPrefix(p));  // Can theoretically throw, but should not.
}

ComponentSymbolVariantItemSuffix EagleTypeConverter::convertGateName(
    const QString& n) {
  return ComponentSymbolVariantItemSuffix(
      cleanComponentSymbolVariantItemSuffix(n));
}

CircuitIdentifier EagleTypeConverter::convertPinOrPadName(const QString& n) {
  QString name = convertInversionSyntax(cleanCircuitIdentifier(n));
  if ((name.length() > 2) && (name.startsWith("P$"))) {
    name.remove(0, 2);
  }
  if (name.isEmpty()) {
    name = "Unnamed";
  }
  return CircuitIdentifier(name);
}

QString EagleTypeConverter::convertInversionSyntax(const QString& s) noexcept {
  QString out;
  bool inputOverlined = false;
  bool outputOverlined = false;
  for (int i = 0; i < s.count(); ++i) {
    if (s.at(i) == '!') {
      inputOverlined = !inputOverlined;
      continue;
    }
    if (s.at(i) == '/') {
      outputOverlined = false;
    }
    if (inputOverlined != outputOverlined) {
      out += "!";
      outputOverlined = inputOverlined;
    }
    out += s.at(i);
  }
  return out;
}

std::shared_ptr<Attribute> EagleTypeConverter::tryConvertAttribute(
    const parseagle::Attribute& a, MessageLogger& log) {
  const QString key = cleanAttributeKey(a.getName());
  if (key.isEmpty()) {
    log.warning(QString("Skipped attribute '%1' due to invalid name.")
                    .arg(a.getName()));
    return nullptr;
  }
  return std::make_shared<Attribute>(
      AttributeKey(key), AttrTypeString::instance(), a.getValue(), nullptr);
}

void EagleTypeConverter::tryConvertAttributes(
    const QList<parseagle::Attribute>& in, AttributeList& out,
    MessageLogger& log) {
  foreach (const auto& eagleAttr, in) {
    if (auto lpObj = tryConvertAttribute(eagleAttr, log)) {
      if (!out.contains(*lpObj->getKey())) {
        out.append(lpObj);
      }
    }
  }
}

void EagleTypeConverter::tryExtractMpnAndManufacturer(
    AttributeList& attributes, SimpleString& mpn,
    SimpleString& manufacturer) noexcept {
  for (auto name : {"MPN", "MANUFACTURER_PART_NUMBER", "PART_NUMBER"}) {
    if (auto a = attributes.find(name)) {
      if (mpn->isEmpty()) {
        mpn = cleanSimpleString(a->getValue());
        attributes.remove(name);
      }
    }
  }
  for (auto name : {"MANUFACTURER", "MFR", "MF", "VENDOR"}) {
    if (auto a = attributes.find(name)) {
      if (manufacturer->isEmpty()) {
        manufacturer = cleanSimpleString(a->getValue());
        attributes.remove(name);
      }
    }
  }
}

const Layer* EagleTypeConverter::tryConvertSchematicLayer(int id) noexcept {
  switch (id) {
    case 90:  // modules
      // Not sure what this layer is used for, discard or not?!
      return &Layer::schematicDocumentation();
    case 91:  // nets
      // In some schematics, this layer seems to be used for things not related
      // to nets at all so let's move them to the documentation layer.
      return &Layer::schematicDocumentation();
    case 92:  // buses
      // Probably the same as for layer 91?
      return &Layer::schematicDocumentation();
    case 93:  // pins
      return &Layer::symbolPinNames();
    case 94:  // symbols
      return &Layer::symbolOutlines();
    case 95:  // names
      return &Layer::symbolNames();
    case 96:  // values
      return &Layer::symbolValues();
    case 97:  // info
      return &Layer::schematicDocumentation();
    case 98:  // guide
      return &Layer::schematicGuide();
    case 99:  // spice order
      return nullptr;
    default:
      return nullptr;
  }
}

const Layer* EagleTypeConverter::tryConvertBoardLayer(int id) noexcept {
  switch (id) {
    case 1:  // tCu
      return &Layer::topCopper();
    case 2:  // inner copper
    case 3:  // inner copper
    case 4:  // inner copper
    case 5:  // inner copper
    case 6:  // inner copper
    case 7:  // inner copper
    case 8:  // inner copper
    case 9:  // inner copper
    case 10:  // inner copper
    case 11:  // inner copper
    case 12:  // inner copper
    case 13:  // inner copper
    case 14:  // inner copper
    case 15:  // inner copper
      return Layer::innerCopper(id - 1);
    case 16:  // bCu
      return &Layer::botCopper();
    case 17:  // pads
      break;
    case 18:  // vias
      break;
    case 19:  // unrouted
      break;
    case 20:  // dimension
      // Note: We cannot know whether we need to return Layer::boardOutlines()
      // or Layer::boardCutouts(), but for footprints the dimension layer is
      // more likely used for cutouts.
      return &Layer::boardCutouts();
    case 21:  // tPlace
      return &Layer::topLegend();
    case 22:  // bPlace
      return &Layer::botLegend();
    case 23:  // tOrigins
      break;
    case 24:  // bOrigins
      break;
    case 25:  // tNames
      return &Layer::topNames();
    case 26:  // bNames
      return &Layer::botNames();
    case 27:  // tValues
      return &Layer::topValues();
    case 28:  // bValues
      return &Layer::botValues();
    case 29:  // tStop
      return &Layer::topStopMask();
    case 30:  // bStop
      return &Layer::botStopMask();
    case 31:  // tCream
      return &Layer::topSolderPaste();
    case 32:  // bCream
      return &Layer::botSolderPaste();
    case 33:  // tFinish
      break;
    case 34:  // bFinish
      break;
    case 35:  // tGlue
      return &Layer::topGlue();
    case 36:  // bGlue
      return &Layer::botGlue();
    case 37:  // tTest
      break;
    case 38:  // bTest
      break;
    case 39:  // tKeepout
      break;
    case 40:  // bKeepout
      break;
    case 41:  // tRestrict
      break;
    case 42:  // bRestrict
      break;
    case 43:  // vRestrict
      break;
    case 44:  // drills
      break;
    case 45:  // holes
      break;
    case 46:  // milling
      return &Layer::boardCutouts();
    case 47:  // measures
      return &Layer::boardDocumentation();
    case 48:  // document
      return &Layer::boardDocumentation();
    case 49:  // ReferenceLC
      return &Layer::boardDocumentation();
    case 50:  // ReferenceLS
      return &Layer::boardDocumentation();
    case 51:  // tDocu
      return &Layer::topDocumentation();
    case 52:  // bDocu
      return &Layer::botDocumentation();
    default:
      break;
  }
  return nullptr;
}

QHash<const Layer*, const Layer*> EagleTypeConverter::convertLayerSetup(
    const QString& s) {
  QString tmp = s;
  tmp.replace(QRegularExpression("[:\\*+\\(\\)\\[\\]]"), " ");
  QSet<int> numbers;
  foreach (const QString& numberStr,
           tmp.split(" ", QtCompat::skipEmptyParts())) {
    const int id = numberStr.toInt();
    if ((id < 1) || (id > 16)) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Unsupported layer setup: %1").arg(s));
    }
    numbers.insert(id);
  }
  QHash<const Layer*, const Layer*> result;
  int nextInnerLayer = 1;
  foreach (int id, Toolbox::sortedQSet(numbers)) {
    if (id == 1) {
      result.insert(&Layer::topCopper(), &Layer::topCopper());
    } else if (id == 16) {
      result.insert(&Layer::botCopper(), &Layer::botCopper());
    } else {
      result.insert(Layer::innerCopper(id - 1),
                    Layer::innerCopper(nextInnerLayer));
      ++nextInnerLayer;
    }
  }
  return result;
}

Alignment EagleTypeConverter::convertAlignment(parseagle::Alignment a) {
  switch (a) {
    case parseagle::Alignment::BottomLeft:
      return Alignment(HAlign::left(), VAlign::bottom());
    case parseagle::Alignment::BottomCenter:
      return Alignment(HAlign::center(), VAlign::bottom());
    case parseagle::Alignment::BottomRight:
      return Alignment(HAlign::right(), VAlign::bottom());
    case parseagle::Alignment::CenterLeft:
      return Alignment(HAlign::left(), VAlign::center());
    case parseagle::Alignment::Center:
      return Alignment(HAlign::center(), VAlign::center());
    case parseagle::Alignment::CenterRight:
      return Alignment(HAlign::right(), VAlign::center());
    case parseagle::Alignment::TopLeft:
      return Alignment(HAlign::left(), VAlign::top());
    case parseagle::Alignment::TopCenter:
      return Alignment(HAlign::center(), VAlign::top());
    case parseagle::Alignment::TopRight:
      return Alignment(HAlign::right(), VAlign::top());
    default:
      return Alignment(HAlign::left(), VAlign::bottom());
  }
}

Length EagleTypeConverter::convertLength(double l) {
  return Length::fromMm(l);
}

UnsignedLength EagleTypeConverter::convertLineWidth(double w, int layerId) {
  Length l;
  switch (layerId) {
    case 20:  // dimension
    case 46:  // milling
      l = 0;
      break;
    default:
      l = convertLength(w);
      break;
  }
  return UnsignedLength(l);  // can throw
}

template <>
Length EagleTypeConverter::convertParamTo(const parseagle::Param& p) {
  const QMap<QString, LengthUnit> unitMap = {
      {"mic", LengthUnit::micrometers()},
      {"mm", LengthUnit::millimeters()},
      {"mil", LengthUnit::mils()},
      {"inch", LengthUnit::inches()},
  };

  double value;
  QString unit;
  if (p.tryGetValueAsDoubleWithUnit(value, unit) && unitMap.contains(unit)) {
    return unitMap[unit].convertFromUnit(value);
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid length parameter value: '%1'").arg(p.getValue()));
  }
}

template <>
UnsignedLength EagleTypeConverter::convertParamTo(const parseagle::Param& p) {
  return UnsignedLength(convertParamTo<Length>(p));
}

template <>
PositiveLength EagleTypeConverter::convertParamTo(const parseagle::Param& p) {
  return PositiveLength(convertParamTo<Length>(p));
}

template <>
Ratio EagleTypeConverter::convertParamTo(const parseagle::Param& p) {
  double value;
  if (p.tryGetValueAsDouble(value)) {
    return Ratio::fromNormalized(value);
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid ratio parameter value: '%1'").arg(p.getValue()));
  }
}

template <>
UnsignedRatio EagleTypeConverter::convertParamTo(const parseagle::Param& p) {
  return UnsignedRatio(convertParamTo<Ratio>(p));
}

Point EagleTypeConverter::convertPoint(const parseagle::Point& p) {
  return Point::fromMm(p.x, p.y);
}

Angle EagleTypeConverter::convertAngle(double a) {
  return Angle::fromDeg(a);
}

void EagleTypeConverter::convertGrid(const parseagle::Grid& g,
                                     PositiveLength& interval,
                                     LengthUnit& unit) {
  const QMap<parseagle::GridUnit, LengthUnit> unitMap = {
      {parseagle::GridUnit::Micrometers, LengthUnit::micrometers()},
      {parseagle::GridUnit::Millimeters, LengthUnit::millimeters()},
      {parseagle::GridUnit::Mils, LengthUnit::mils()},
      {parseagle::GridUnit::Inches, LengthUnit::inches()},
  };
  const LengthUnit distUnit =
      unitMap.value(g.getUnitDistance(), LengthUnit::millimeters());
  const Length value = distUnit.convertFromUnit(g.getDistance());
  if ((g.getDistance() > 0) && (value > 0)) {
    interval = PositiveLength(value);
  }
  if (unitMap.contains(g.getUnit())) {
    unit = unitMap.value(g.getUnit());
  }
}

Vertex EagleTypeConverter::convertVertex(const parseagle::Vertex& v) {
  return Vertex(convertPoint(v.getPosition()), convertAngle(v.getCurve()));
}

Path EagleTypeConverter::convertVertices(const QList<parseagle::Vertex>& v,
                                         bool close) {
  Path path;
  foreach (const parseagle::Vertex& vertex, v) {
    path.addVertex(convertVertex(vertex));
  }
  if (close) {
    path.close();
  }
  return path;
}

QList<EagleTypeConverter::Geometry> EagleTypeConverter::convertAndJoinWires(
    const QList<parseagle::Wire>& wires, bool isGrabAreaIfClosed,
    MessageLogger& log) {
  QMap<std::pair<int, double>, QList<parseagle::Wire>> joinableWires;
  foreach (const parseagle::Wire& wire, wires) {
    auto key = std::make_pair(wire.getLayer(), wire.getWidth());
    joinableWires[key].append(wire);
  }

  QList<Geometry> polygons;
  bool timedOut = false;
  for (auto it = joinableWires.begin(); it != joinableWires.end(); it++) {
    try {
      QVector<Path> paths;
      foreach (const parseagle::Wire& wire, it.value()) {
        paths.append(Path::line(convertPoint(wire.getP1()),
                                convertPoint(wire.getP2()),
                                convertAngle(wire.getCurve())));
        if (wire.getWireStyle() != parseagle::WireStyle::Continuous) {
          log.warning(
              tr("Dashed/dotted line is not supported, converting to "
                 "continuous."));
        }
        if (wire.getWireCap() != parseagle::WireCap::Round) {
          log.warning(
              tr("Flat line end is not supported, converting to round."));
        }
      }
      foreach (const Path& p, TangentPathJoiner::join(paths, 5000, &timedOut)) {
        polygons.append(Geometry{
            it.value().first().getLayer(),  // Layer
            convertLineWidth(it.value().first().getWidth(),
                             it.value().first().getLayer()),  // Line width
            false,  // Filled
            isGrabAreaIfClosed && p.isClosed(),  // Grab area
            p,  // Path
            tl::nullopt,  // Circle
        });
      }
    } catch (const Exception& e) {
      log.warning(QString("Failed to convert wires: %1").arg(e.getMsg()));
    }
  }
  if (timedOut) {
    log.info(
        "Aborted joining tangent line segments to polygons due to timeout, "
        "keeping them separate.");
  }
  return polygons;
}

EagleTypeConverter::Geometry EagleTypeConverter::convertRectangle(
    const parseagle::Rectangle& r, bool isGrabArea) {
  const Point p1 = convertPoint(r.getP1());
  const Point p2 = convertPoint(r.getP2());
  const Point center = (p1 + p2) / 2;
  const Angle rotation = convertAngle(r.getRotation().getAngle());
  const Path path = Path::rect(p1, p2).rotated(rotation, center);
  return Geometry{
      r.getLayer(),  // Layer
      UnsignedLength(0),  // Line width
      true,  // Filled
      isGrabArea,  // Grab area
      path,  // Path
      tl::nullopt,  // Circle
  };
}

EagleTypeConverter::Geometry EagleTypeConverter::convertPolygon(
    const parseagle::Polygon& p, bool isGrabArea) {
  return Geometry{
      p.getLayer(),  // Layer
      convertLineWidth(p.getWidth(), p.getLayer()),  // Line width
      true,  // Filled (EAGLE polygons are always filled)
      isGrabArea,  // Grab area
      convertVertices(p.getVertices(), true),  // Path (polygons are closed)
      tl::nullopt,  // Circle
  };
}

EagleTypeConverter::Geometry EagleTypeConverter::convertCircle(
    const parseagle::Circle& c, bool isGrabArea) {
  const bool filled = (c.getWidth() == 0);  // EAGLE fills zero-width circles!
  const UnsignedLength lineWidth = convertLineWidth(c.getWidth(), c.getLayer());
  const Point pos = convertPoint(c.getPosition());
  const PositiveLength diameter(convertLength(c.getRadius()) * 2);
  return Geometry{
      c.getLayer(),  // Layer
      lineWidth,  // Line width
      filled,  // Filled
      isGrabArea,  // Grab area
      Path::circle(diameter).translated(pos),
      std::make_pair(pos, diameter),  // Circle
  };
}

std::shared_ptr<Hole> EagleTypeConverter::convertHole(
    const parseagle::Hole& h) {
  return std::make_shared<Hole>(
      Uuid::createRandom(),  // UUID
      PositiveLength(convertLength(h.getDiameter())),  // Diameter
      makeNonEmptyPath(convertPoint(h.getPosition())),  // Path
      MaskConfig::automatic()  // Stop mask
  );
}

EagleTypeConverter::Geometry EagleTypeConverter::convertFrame(
    const parseagle::Frame& f) {
  const Length width(3810000);
  const Point p1 = convertPoint(f.getP1());
  const Point p2 = convertPoint(f.getP2());
  const Point p1Abs(std::min(p1.getX(), p2.getX()) + width,
                    std::min(p1.getY(), p2.getY()) + width);
  const Point p2Abs(std::max(p1.getX(), p2.getX()) - width,
                    std::max(p1.getY(), p2.getY()) - width);
  return Geometry{
      f.getLayer(),  // Layer
      UnsignedLength(200000),  // Line width
      false,  // Filled
      false,  // Grab area
      Path::rect(p1Abs, p2Abs),  // Path
      tl::nullopt,  // Circle
  };
}

QString EagleTypeConverter::convertTextValue(const QString& v) {
  QString value = v;
  if (value == ">DRAWING_NAME") {
    value = "{{PROJECT}}";
  } else if ((value == ">LAST_DATE_TIME") || (value == ">PLOT_DATE_TIME")) {
    value = "{{DATE}} {{TIME}}";
  } else if (value == ">SHEET") {
    value = "{{PAGE}}/{{PAGES}}";
  } else if (value.startsWith(">")) {
    value = "{{" + value.mid(1).toUpper() + "}}";
  }
  return value;
}

PositiveLength EagleTypeConverter::convertSchematicTextSize(double s) {
  return PositiveLength(Length::fromMm(s * 2.5 / 1.778));
}

std::shared_ptr<Text> EagleTypeConverter::tryConvertSchematicText(
    const parseagle::Text& t) {
  if (auto layer = tryConvertSchematicLayer(t.getLayer())) {
    const bool mirror = t.getRotation().getMirror();
    const Angle rotation = convertAngle(t.getRotation().getAngle());
    const Alignment alignment = convertAlignment(t.getAlignment());
    return std::make_shared<Text>(
        Uuid::createRandom(),  // UUID
        *layer,  // Layer
        convertTextValue(t.getValue()),  // Text
        convertPoint(t.getPosition()),  // Position
        mirror ? -rotation : rotation,  // Rotation
        convertSchematicTextSize(t.getSize()),  // Height
        mirror ? alignment.mirroredH() : alignment  // Alignment
    );
  } else {
    return nullptr;
  }
}

std::shared_ptr<Text> EagleTypeConverter::tryConvertSchematicAttribute(
    const parseagle::Attribute& t) {
  if (auto layer = tryConvertSchematicLayer(t.getLayer())) {
    const bool mirror = t.getRotation().getMirror();
    const Angle rotation = convertAngle(t.getRotation().getAngle());
    const Alignment alignment = convertAlignment(t.getAlignment());
    return std::make_shared<Text>(
        Uuid::createRandom(),  // UUID
        *layer,  // Layer
        convertTextValue(">" % t.getName()),  // Text
        convertPoint(t.getPosition()),  // Position
        mirror ? -rotation : rotation,  // Rotation
        convertSchematicTextSize(t.getSize()),  // Height
        mirror ? alignment.mirroredH() : alignment  // Alignment
    );
  } else {
    return nullptr;
  }
}

PositiveLength EagleTypeConverter::convertBoardTextSize(int layerId,
                                                        double size) {
  Length newSize = Length::fromMm(size * 0.85);
  // Avoid too small texts on silkscreen layers. Do not touch texts on
  // functional layers like copper to avoid possible unintended effects.
  switch (layerId) {
    case 21:  // tPlace
    case 22:  // bPlace
    case 25:  // tNames
    case 26:  // bNames
    case 27:  // tValues
    case 28:  // bValues
      newSize = std::max(newSize, Length(800000));  // min. 0.8mm
      break;
    default:
      break;
  }
  return PositiveLength(newSize);
}

UnsignedLength EagleTypeConverter::convertBoardTextStrokeWidth(int layerId,
                                                               double size,
                                                               int ratio) {
  if (ratio == 0) {
    ratio = 15;
  }
  Length width = Length::fromMm((size * ratio) / 100);
  // Avoid too thin texts on silkscreen layers. Do not touch texts on
  // functional layers like copper to avoid possible unintended effects.
  switch (layerId) {
    case 21:  // tPlace
    case 22:  // bPlace
    case 25:  // tNames
    case 26:  // bNames
    case 27:  // tValues
    case 28:  // bValues
      width = std::max(width, Length(150000));  // min. 150um
      break;
    default:
      break;
  }
  return UnsignedLength(width);
}

std::shared_ptr<StrokeText> EagleTypeConverter::tryConvertBoardText(
    const parseagle::Text& t) {
  if (auto layer = tryConvertBoardLayer(t.getLayer())) {
    const bool mirror = t.getRotation().getMirror();
    const Angle rotation = convertAngle(t.getRotation().getAngle());
    return std::make_shared<StrokeText>(
        Uuid::createRandom(),  // UUID
        *layer,  // Layer
        convertTextValue(t.getValue()),  // Text
        convertPoint(t.getPosition()),  // Position
        mirror ? -rotation : rotation,  // Rotation
        convertBoardTextSize(t.getLayer(), t.getSize()),  // Height
        convertBoardTextStrokeWidth(t.getLayer(), t.getSize(),
                                    t.getRatio()),  // Stroke width
        StrokeTextSpacing(),  // Letter spacing
        StrokeTextSpacing(),  // Line spacing
        convertAlignment(t.getAlignment()),  // Alignment
        mirror,  // Mirrored
        !t.getRotation().getSpin()  // Auto rotate
    );
  } else {
    return nullptr;
  }
}

std::shared_ptr<StrokeText> EagleTypeConverter::tryConvertBoardAttribute(
    const parseagle::Attribute& t) {
  if (auto layer = tryConvertBoardLayer(t.getLayer())) {
    const bool mirror = t.getRotation().getMirror();
    const Angle rotation = convertAngle(t.getRotation().getAngle());
    return std::make_shared<StrokeText>(
        Uuid::createRandom(),  // UUID
        *layer,  // Layer
        convertTextValue(">" % t.getName()),  // Text
        convertPoint(t.getPosition()),  // Position
        mirror ? -rotation : rotation,  // Rotation
        convertBoardTextSize(t.getLayer(), t.getSize()),  // Height
        convertBoardTextStrokeWidth(t.getLayer(), t.getSize(),
                                    t.getRatio()),  // Stroke width
        StrokeTextSpacing(),  // Letter spacing
        StrokeTextSpacing(),  // Line spacing
        convertAlignment(t.getAlignment()),  // Alignment
        mirror,  // Mirrored
        !t.getRotation().getSpin()  // Auto rotate
    );
  } else {
    return nullptr;
  }
}

EagleTypeConverter::Pin EagleTypeConverter::convertSymbolPin(
    const parseagle::Pin& p) {
  Pin result;
  const bool isDot = (p.getFunction() == parseagle::PinFunction::Dot) ||
      (p.getFunction() == parseagle::PinFunction::DotClock);
  const bool isClock = (p.getFunction() == parseagle::PinFunction::Clock) ||
      (p.getFunction() == parseagle::PinFunction::DotClock);
  const UnsignedLength dotDiameter(isDot ? 1700000 : 0);
  const UnsignedLength totalLength(convertLength(p.getLengthInMillimeters()));
  result.pin = std::make_shared<SymbolPin>(
      Uuid::createRandom(),  // UUID
      convertPinOrPadName(p.getName()),  // Name
      convertPoint(p.getPosition()),  // Position
      UnsignedLength(
          std::max(*totalLength - *dotDiameter, Length(0))),  // Length
      convertAngle(p.getRotation().getAngle()),  // Rotation
      Point(totalLength + Length(2540000), 0),  // Name position
      Angle(0),  // Name rotation
      SymbolPin::getDefaultNameHeight(),  // Name height
      Alignment(HAlign::left(), VAlign::center())  // Name alignment
  );
  if (isDot) {
    result.circle =
        std::make_shared<Circle>(Uuid::createRandom(), Layer::symbolOutlines(),
                                 UnsignedLength(158750), false, false,
                                 Point((*totalLength) - (*dotDiameter) / 2, 0)
                                         .rotated(result.pin->getRotation()) +
                                     result.pin->getPosition(),
                                 PositiveLength(*dotDiameter));
  }
  if (isClock) {
    const Length dy(900000);
    const Length dx(1900000);
    const Path path = Path({
                               Vertex(Point(*totalLength, dy)),
                               Vertex(Point(*totalLength + dx, 0)),
                               Vertex(Point(*totalLength, -dy)),
                           })
                          .rotated(result.pin->getRotation())
                          .translated(result.pin->getPosition());
    result.polygon =
        std::make_shared<Polygon>(Uuid::createRandom(), Layer::symbolOutlines(),
                                  UnsignedLength(158750), false, false, path);
  }
  return result;
}

std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad>>
    EagleTypeConverter::convertThtPad(
        const parseagle::ThtPad& p,
        const BoundedUnsignedRatio& autoAnnularWidth) {
  Uuid uuid = Uuid::createRandom();
  const PositiveLength drillDiameter(convertLength(p.getDrillDiameter()));
  Length size = convertLength(p.getOuterDiameter());
  if (size <= 0) {
    // If the pad size is set to "auto", it will be zero.
    const UnsignedLength annular = autoAnnularWidth.calcValue(*drillDiameter);
    size = drillDiameter + annular * 2;
  }
  PositiveLength width(size);
  PositiveLength height(size);
  UnsignedLimitedRatio radius(Ratio::fromPercent(0));
  FootprintPad::Shape shape;
  Path customShapeOutline;
  switch (p.getShape()) {
    case parseagle::PadShape::Square:
      shape = FootprintPad::Shape::RoundedRect;
      break;
    case parseagle::PadShape::Octagon:
      shape = FootprintPad::Shape::RoundedOctagon;
      break;
    case parseagle::PadShape::Round:
      shape = FootprintPad::Shape::RoundedRect;
      radius = UnsignedLimitedRatio(Ratio::fromPercent(100));
      break;
    case parseagle::PadShape::Long:
      shape = FootprintPad::Shape::RoundedRect;
      radius = UnsignedLimitedRatio(Ratio::fromPercent(100));
      width = PositiveLength(size * 2);
      break;
    case parseagle::PadShape::Offset:
      shape = FootprintPad::Shape::Custom;
      radius = UnsignedLimitedRatio(Ratio::fromPercent(100));
      width = PositiveLength(size * 2);
      customShapeOutline =
          Path::obround(width, height).translated(Point(size / 2, 0));
      break;
    default:
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Unknown pad shape: %1").arg(static_cast<int>(p.getShape())));
  }
  return std::make_pair(
      std::make_shared<PackagePad>(uuid,  // UUID
                                   convertPinOrPadName(p.getName())  // Name
                                   ),
      std::make_shared<FootprintPad>(
          uuid,  // UUID
          uuid,  // Package pad UUID
          convertPoint(p.getPosition()),  // Position
          convertAngle(p.getRotation().getAngle()),  // Rotation
          shape,  // Shape
          width,  // Width
          height,  // Height
          radius,  // Radius
          customShapeOutline,  // Custom shape outline
          p.getStop() ? MaskConfig::automatic()
                      : MaskConfig::off(),  // Stop mask
          MaskConfig::off(),  // Solder paste
          UnsignedLength(0),  // Copper clearance
          FootprintPad::ComponentSide::Top,  // Side
          FootprintPad::Function::Unspecified,  // Function
          PadHoleList{std::make_shared<PadHole>(
              Uuid::createRandom(), drillDiameter,
              makeNonEmptyPath(Point(0, 0)))}  // Holes
          ));
}

std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad>>
    EagleTypeConverter::convertSmtPad(const parseagle::SmtPad& p) {
  Uuid uuid = Uuid::createRandom();
  const Layer* layer = tryConvertBoardLayer(p.getLayer());
  FootprintPad::ComponentSide side;
  if (layer == &Layer::topCopper()) {
    side = FootprintPad::ComponentSide::Top;
  } else if (layer == &Layer::botCopper()) {
    side = FootprintPad::ComponentSide::Bottom;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid pad layer: %1").arg(p.getLayer()));
  }
  return std::make_pair(
      std::make_shared<PackagePad>(uuid,  // UUID
                                   convertPinOrPadName(p.getName())  // Name
                                   ),
      std::make_shared<FootprintPad>(
          uuid,  // UUID
          uuid,  // Package pad UUID
          convertPoint(p.getPosition()),  // Position
          convertAngle(p.getRotation().getAngle()),  // Rotation
          FootprintPad::Shape::RoundedRect,  // Shape
          PositiveLength(convertLength(p.getWidth())),  // Width
          PositiveLength(convertLength(p.getHeight())),  // Height
          UnsignedLimitedRatio(Ratio::fromPercent(p.getRoundness())),  // Radius
          Path(),  // Custom shape outline
          p.getStop() ? MaskConfig::automatic()
                      : MaskConfig::off(),  // Stop mask
          p.getCream() ? MaskConfig::automatic()
                       : MaskConfig::off(),  // Solder paste
          UnsignedLength(0),  // Copper clearance
          side,  // Side
          FootprintPad::Function::Unspecified,  // Function
          PadHoleList{}  // Holes
          ));
}

std::shared_ptr<Circle> EagleTypeConverter::tryConvertToSchematicCircle(
    const Geometry& g) {
  if (g.circle) {
    if (auto layer = tryConvertSchematicLayer(g.layerId)) {
      return std::make_shared<Circle>(Uuid::createRandom(), *layer, g.lineWidth,
                                      g.filled, g.grabArea, g.circle->first,
                                      g.circle->second);
    }
  }
  return nullptr;
}

std::shared_ptr<Polygon> EagleTypeConverter::tryConvertToSchematicPolygon(
    const Geometry& g) {
  if (auto layer = tryConvertSchematicLayer(g.layerId)) {
    return std::make_shared<Polygon>(Uuid::createRandom(), *layer, g.lineWidth,
                                     g.filled, g.grabArea, g.path);
  }
  return nullptr;
}

QVector<Path> EagleTypeConverter::convertBoardZoneOutline(
    const Path& outline, const Length& lineWidth) {
  const PositiveLength maxArcTolerance(10000);
  if ((lineWidth / 2) > (*maxArcTolerance)) {
    ClipperLib::Paths paths{ClipperHelpers::convert(outline, maxArcTolerance)};
    ClipperHelpers::offset(paths, lineWidth / 2, maxArcTolerance,
                           ClipperLib::jtRound);
    return ClipperHelpers::convert(paths);
  } else {
    return {outline};
  }
}

QVector<std::shared_ptr<Zone>> EagleTypeConverter::tryConvertToBoardZones(
    const Geometry& g) {
  Zone::Layers layers = Zone::Layers();
  Zone::Rules rules = Zone::Rules();
  switch (g.layerId) {
    case 39:  // tKeepout
      layers = Zone::Layer::Top;
      rules = Zone::Rule::NoDevices;
      break;
    case 40:  // bKeepout
      layers = Zone::Layer::Bottom;
      rules = Zone::Rule::NoDevices;
      break;
    case 41:  // tRestrict
      layers = Zone::Layer::Top;
      rules = Zone::Rule::NoCopper | Zone::Rule::NoPlanes;
      break;
    case 42:  // bRestrict
      layers = Zone::Layer::Bottom;
      rules = Zone::Rule::NoCopper | Zone::Rule::NoPlanes;
      break;
    case 43:  // vRestrict
      layers = Zone::Layer::Inner;
      rules = Zone::Rule::NoCopper | Zone::Rule::NoPlanes;
      break;
    default:
      return {};
  }
  QVector<std::shared_ptr<Zone>> result;
  foreach (const Path& outline, convertBoardZoneOutline(g.path, *g.lineWidth)) {
    result.append(
        std::make_shared<Zone>(Uuid::createRandom(), layers, rules, outline));
  }
  return result;
}

std::shared_ptr<Circle> EagleTypeConverter::tryConvertToBoardCircle(
    const Geometry& g) {
  if (g.circle) {
    if (auto layer = tryConvertBoardLayer(g.layerId)) {
      return std::make_shared<Circle>(Uuid::createRandom(), *layer, g.lineWidth,
                                      g.filled, g.grabArea, g.circle->first,
                                      g.circle->second);
    }
  }
  return nullptr;
}

std::shared_ptr<Polygon> EagleTypeConverter::tryConvertToBoardPolygon(
    const Geometry& g) {
  if (auto layer = tryConvertBoardLayer(g.layerId)) {
    return std::make_shared<Polygon>(Uuid::createRandom(), *layer, g.lineWidth,
                                     g.filled, g.grabArea, g.path);
  }
  return nullptr;
}

QString EagleTypeConverter::getLayerName(int id,
                                         const QString& fallback) noexcept {
  switch (id) {
    case 1:
      return "tCu";
    case 2:  // inner copper
    case 3:  // inner copper
    case 4:  // inner copper
    case 5:  // inner copper
    case 6:  // inner copper
    case 7:  // inner copper
    case 8:  // inner copper
    case 9:  // inner copper
    case 10:  // inner copper
    case 11:  // inner copper
    case 12:  // inner copper
    case 13:  // inner copper
    case 14:  // inner copper
    case 15:  // inner copper
      return QString("Route%1").arg(id);
    case 16:
      return "bCu";
    case 17:
      return "Pads";
    case 18:
      return "Vias";
    case 19:
      return "Unrouted";
    case 20:
      return "Dimension";
    case 21:
      return "tPlace";
    case 22:
      return "bPlace";
    case 23:
      return "tOrigins";
    case 24:
      return "bOrigins";
    case 25:
      return "tNames";
    case 26:
      return "bNames";
    case 27:
      return "tValues";
    case 28:
      return "bValues";
    case 29:
      return "tStop";
    case 30:
      return "bStop";
    case 31:
      return "tCream";
    case 32:
      return "bCream";
    case 33:
      return "tFinish";
    case 34:
      return "bFinish";
    case 35:
      return "tGlue";
    case 36:
      return "bGlue";
    case 37:
      return "tTest";
    case 38:
      return "bTest";
    case 39:
      return "tKeepout";
    case 40:
      return "bKeepout";
    case 41:
      return "tRestrict";
    case 42:
      return "bRestrict";
    case 43:
      return "vRestrict";
    case 44:
      return "Drills";
    case 45:
      return "Holes";
    case 46:
      return "Milling";
    case 47:
      return "Measures";
    case 48:
      return "Document";
    case 49:
      return "ReferenceLC";
    case 50:
      return "ReferenceLS";
    case 51:
      return "tDocu";
    case 52:
      return "bDocu";
    case 90:
      return "Modules";
    case 91:
      return "Nets";
    case 92:
      return "Buses";
    case 93:
      return "Pins";
    case 94:
      return "Symbols";
    case 95:
      return "Names";
    case 96:
      return "Values";
    case 97:
      return "Info";
    case 98:
      return "Guide";
    case 99:
      return "Spice Order";
    default:
      return fallback;
  }
}

BoundedUnsignedRatio
    EagleTypeConverter::getDefaultAutoThtAnnularWidth() noexcept {
  // The EAGLE footprint editor displays an annular ring of 25% of the drill
  // diameter then, bounded to 10..20mils (0.254..0.508mm).
  return BoundedUnsignedRatio(UnsignedRatio(Ratio::fromPercent(25)),
                              UnsignedLength(254000), UnsignedLength(508000));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb
