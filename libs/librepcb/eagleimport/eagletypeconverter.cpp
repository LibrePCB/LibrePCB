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

#include <librepcb/core/utils/tangentpathjoiner.h>
#include <parseagle/common/circle.h>
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
      .split("\n", QString::SkipEmptyParts)
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
    if (s.at(i) == "!") {
      inputOverlined = !inputOverlined;
      continue;
    }
    if (s.at(i) == "/") {
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
      break;
    case 36:  // bGlue
      return &Layer::botGlue();
      break;
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

Length EagleTypeConverter::convertLength(double l) {
  return Length::fromMm(l);
}

Point EagleTypeConverter::convertPoint(const parseagle::Point& p) {
  return Point::fromMm(p.x, p.y);
}

Angle EagleTypeConverter::convertAngle(double a) {
  return Angle::fromDeg(a);
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
    QStringList* errors) {
  QMap<std::pair<int, double>, QList<parseagle::Wire>> joinableWires;
  foreach (const parseagle::Wire& wire, wires) {
    auto key = std::make_pair(wire.getLayer(), wire.getWidth());
    joinableWires[key].append(wire);
  }

  QList<Geometry> polygons;
  for (auto it = joinableWires.begin(); it != joinableWires.end(); it++) {
    try {
      QVector<Path> paths;
      foreach (const parseagle::Wire& wire, it.value()) {
        paths.append(Path::line(convertPoint(wire.getP1()),
                                convertPoint(wire.getP2()),
                                convertAngle(wire.getCurve())));
      }
      foreach (const Path& path, TangentPathJoiner::join(paths, 5000)) {
        polygons.append(Geometry{
            it.value().first().getLayer(),  // Layer
            UnsignedLength(
                convertLength(it.value().first().getWidth())),  // Line width
            false,  // Filled
            isGrabAreaIfClosed && path.isClosed(),  // Grab area
            path,  // Path
            tl::nullopt,  // Circle
        });
      }
    } catch (const Exception& e) {
      if (errors) errors->append(e.getMsg());
    }
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
      UnsignedLength(convertLength(p.getWidth())),  // Line width
      true,  // Filled (EAGLE polygons are always filled)
      isGrabArea,  // Grab area
      convertVertices(p.getVertices(), true),  // Path (polygons are closed)
      tl::nullopt,  // Circle
  };
}

EagleTypeConverter::Geometry EagleTypeConverter::convertCircle(
    const parseagle::Circle& c, bool isGrabArea) {
  const bool filled = (c.getWidth() == 0);  // EAGLE fills zero-width circles!
  const UnsignedLength lineWidth = UnsignedLength(convertLength(c.getWidth()));
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

QString EagleTypeConverter::convertTextValue(const QString& v) {
  QString value = v;
  if (value.startsWith(">")) {
    value = "{{" + value.mid(1).toUpper() + "}}";
  }
  return value;
}

std::shared_ptr<Text> EagleTypeConverter::tryConvertSchematicText(
    const parseagle::Text& t) {
  if (auto layer = tryConvertSchematicLayer(t.getLayer())) {
    return std::make_shared<Text>(
        Uuid::createRandom(),  // UUID
        *layer,  // Layer
        convertTextValue(t.getValue()),  // Text
        convertPoint(t.getPosition()),  // Position
        convertAngle(t.getRotation().getAngle()),  // Rotation
        PositiveLength(2500000),  // Height
        Alignment(HAlign::left(), VAlign::bottom())  // Alignment
    );
  } else {
    return nullptr;
  }
}

std::shared_ptr<StrokeText> EagleTypeConverter::tryConvertBoardText(
    const parseagle::Text& t) {
  if (auto layer = tryConvertBoardLayer(t.getLayer())) {
    return std::make_shared<StrokeText>(
        Uuid::createRandom(),  // UUID
        *layer,  // Layer
        convertTextValue(t.getValue()),  // Text
        convertPoint(t.getPosition()),  // Position
        convertAngle(t.getRotation().getAngle()),  // Rotation
        PositiveLength(1000000),  // Height
        UnsignedLength(200000),  // Stroke width
        StrokeTextSpacing(),  // Letter spacing
        StrokeTextSpacing(),  // Line spacing
        Alignment(HAlign::left(), VAlign::bottom()),  // Alignment
        false,  // Mirrored
        true  // Auto rotate
    );
  } else {
    return nullptr;
  }
}

std::shared_ptr<SymbolPin> EagleTypeConverter::convertSymbolPin(
    const parseagle::Pin& p) {
  UnsignedLength length(convertLength(p.getLengthInMillimeters()));
  return std::make_shared<SymbolPin>(
      Uuid::createRandom(),  // UUID
      convertPinOrPadName(p.getName()),  // Name
      convertPoint(p.getPosition()),  // Position
      length,  // Length
      convertAngle(p.getRotation().getAngle()),  // Rotation
      SymbolPin::getDefaultNamePosition(length),  // Name position
      Angle(0),  // Name rotation
      SymbolPin::getDefaultNameHeight(),  // Name height
      Alignment(HAlign::left(), VAlign::center())  // Name alignment
  );
}

std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad>>
    EagleTypeConverter::convertThtPad(const parseagle::ThtPad& p) {
  Uuid uuid = Uuid::createRandom();
  Length size = convertLength(p.getOuterDiameter());
  if (size <= 0) {
    // If the pad size is set to "auto", it will be zero.
    size = (convertLength(p.getDrillDiameter()) * 3) / 2;
  }
  PositiveLength width(size);
  PositiveLength height(size);
  UnsignedLimitedRatio radius(Ratio::percent0());
  FootprintPad::Shape shape;
  switch (p.getShape()) {
    case parseagle::ThtPad::Shape::Square:
      shape = FootprintPad::Shape::RoundedRect;
      break;
    case parseagle::ThtPad::Shape::Octagon:
      shape = FootprintPad::Shape::RoundedOctagon;
      break;
    case parseagle::ThtPad::Shape::Round:
      shape = FootprintPad::Shape::RoundedRect;
      radius = UnsignedLimitedRatio(Ratio::percent100());
      break;
    case parseagle::ThtPad::Shape::Long:
      shape = FootprintPad::Shape::RoundedRect;
      radius = UnsignedLimitedRatio(Ratio::percent100());
      width = PositiveLength(size * 2);
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
          Path(),  // Custom shape outline
          MaskConfig::automatic(),  // Stop mask
          MaskConfig::off(),  // Solder paste
          UnsignedLength(0),  // Copper clearance
          FootprintPad::ComponentSide::Top,  // Side
          FootprintPad::Function::Unspecified,  // Function
          PadHoleList{std::make_shared<PadHole>(
              Uuid::createRandom(),
              PositiveLength(convertLength(p.getDrillDiameter())),
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
          UnsignedLimitedRatio(Ratio::percent0()),  // Radius
          Path(),  // Custom shape outline
          MaskConfig::automatic(),  // Stop mask
          MaskConfig::automatic(),  // Solder paste
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb
