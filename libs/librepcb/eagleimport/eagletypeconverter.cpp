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

ComponentSymbolVariantItemSuffix EagleTypeConverter::convertGateName(
    const QString& n) {
  QString suffix = n.trimmed();
  if (suffix.startsWith("G$")) {
    suffix = "";  // Convert EAGLE default name to LibrePCB default name.
  }
  suffix = cleanComponentSymbolVariantItemSuffix(suffix);
  return ComponentSymbolVariantItemSuffix(suffix);
}

CircuitIdentifier EagleTypeConverter::convertPinOrPadName(const QString& n) {
  QString name = cleanCircuitIdentifier(n);
  if ((name.length() > 2) && (name.startsWith("P$"))) {
    name.remove(0, 2);
  }
  if (name.isEmpty()) {
    name = "Unnamed";
  }
  return CircuitIdentifier(name);
}

const Layer& EagleTypeConverter::convertLayer(int id) {
  QString eagleLayerName = "unknown";
  switch (id) {
    case 1:  // tCu
      return Layer::topCopper();
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
      if (const Layer* layer = Layer::innerCopper(id - 1)) {
        return *layer;
      }
      eagleLayerName = "inner";
      break;
    case 16:  // bCu
      return Layer::botCopper();
    case 17:  // pads
      eagleLayerName = "pads";
      break;
    case 18:  // vias
      eagleLayerName = "vias";
      break;
    case 19:  // unrouted
      eagleLayerName = "unrouted";
      break;
    case 20:  // dimension
      return Layer::boardOutlines();
    case 21:  // tPlace
      return Layer::topLegend();
    case 22:  // bPlace
      return Layer::botLegend();
    case 23:  // tOrigins
      eagleLayerName = "tOrigins";
      break;
    case 24:  // bOrigins
      eagleLayerName = "bOrigins";
      break;
    case 25:  // tNames
      return Layer::topNames();
    case 26:  // bNames
      return Layer::botNames();
    case 27:  // tValues
      return Layer::topValues();
    case 28:  // bValues
      return Layer::botValues();
    case 29:  // tStop
      return Layer::topStopMask();
    case 30:  // bStop
      return Layer::botStopMask();
    case 31:  // tCream
      return Layer::topSolderPaste();
    case 32:  // bCream
      return Layer::botSolderPaste();
    case 33:  // tFinish
      eagleLayerName = "tFinish";
      break;
    case 34:  // bFinish
      eagleLayerName = "bFinish";
      break;
    case 35:  // tGlue
      return Layer::topGlue();
    case 36:  // bGlue
      return Layer::botGlue();
    case 37:  // tTest
      eagleLayerName = "tTest";
      break;
    case 38:  // bTest
      eagleLayerName = "bTest";
      break;
    case 39:  // tKeepout
      return Layer::topCourtyard();
    case 40:  // bKeepout
      return Layer::botCourtyard();
    case 41:  // tRestrict
      eagleLayerName = "tRestrict";
      break;
    case 42:  // bRestrict
      eagleLayerName = "bRestrict";
      break;
    case 43:  // vRestrict
      eagleLayerName = "vRestrict";
      break;
    case 44:  // drills
      eagleLayerName = "drills";
      break;
    case 45:  // holes
      eagleLayerName = "holes";
      break;
    case 46:  // milling
      return Layer::boardCutouts();
    case 47:  // measures
      return Layer::boardDocumentation();
    case 48:  // document
      return Layer::boardDocumentation();
    case 49:  // ReferenceLC
      return Layer::boardDocumentation();
    case 50:  // ReferenceLS
      return Layer::boardDocumentation();
    case 51:  // tDocu
      return Layer::topDocumentation();
    case 52:  // bDocu
      return Layer::botDocumentation();
    case 90:  // modules
      eagleLayerName = "modules";
      break;
    case 91:  // nets
      eagleLayerName = "nets";
      break;
    case 92:  // buses
      eagleLayerName = "buses";
      break;
    case 93:  // pins
      return Layer::symbolPinNames();
    case 94:  // symbols
      return Layer::symbolOutlines();
    case 95:  // names
      return Layer::symbolNames();
    case 96:  // values
      return Layer::symbolValues();
    case 97:  // info
      return Layer::schematicDocumentation();
    case 98:  // guide
      return Layer::schematicGuide();
    case 99:  // spice order
      eagleLayerName = "spice order";
      break;
    default:
      break;
  }

  QString layer = QString("%1 (%2)").arg(id).arg(eagleLayerName);
  throw RuntimeError(__FILE__, __LINE__,
                     tr("Layer %1 is not supported.").arg(layer));
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

std::shared_ptr<Polygon> EagleTypeConverter::convertWire(
    const parseagle::Wire& w) {
  return std::make_shared<Polygon>(
      Uuid::createRandom(),  // UUID
      convertLayer(w.getLayer()),  // Layer
      UnsignedLength(convertLength(w.getWidth())),  // Line width
      false,  // Filled
      false,  // Grab area
      Path::line(convertPoint(w.getP1()), convertPoint(w.getP2()),
                 convertAngle(w.getCurve()))  // Path
  );
}

std::shared_ptr<Polygon> EagleTypeConverter::convertRectangle(
    const parseagle::Rectangle& r, bool isGrabArea) {
  Point p1 = convertPoint(r.getP1());
  Point p2 = convertPoint(r.getP2());
  Point center = (p1 + p2) / 2;
  Path path = Path::rect(p1, p2);
  path.rotate(convertAngle(r.getRotation().getAngle()), center);
  return std::make_shared<Polygon>(Uuid::createRandom(),  // UUID
                                   convertLayer(r.getLayer()),  // Layer
                                   UnsignedLength(0),  // Line width
                                   true,  // Filled
                                   isGrabArea,  // Grab area
                                   path  // Path
  );
}

std::shared_ptr<Polygon> EagleTypeConverter::convertPolygon(
    const parseagle::Polygon& p, bool isGrabArea) {
  return std::make_shared<Polygon>(
      Uuid::createRandom(),  // UUID
      convertLayer(p.getLayer()),  // Layer
      UnsignedLength(convertLength(p.getWidth())),  // Line width
      true,  // Filled (EAGLE polygons are always filled)
      isGrabArea,  // Grab area
      convertVertices(p.getVertices(), true)  // Path (polygons are closed)
  );
}

std::shared_ptr<Circle> EagleTypeConverter::convertCircle(
    const parseagle::Circle& c, bool isGrabArea) {
  UnsignedLength lineWidth(convertLength(c.getWidth()));
  bool filled = (lineWidth == 0);  // EAGLE fills circles of zero width!
  return std::make_shared<Circle>(
      Uuid::createRandom(),  // UUID
      convertLayer(c.getLayer()),  // Layer
      lineWidth,  // Line width
      filled,  // Filled
      isGrabArea,  // Grab area
      convertPoint(c.getPosition()),  // Center
      PositiveLength(convertLength(c.getRadius()) * 2)  // Diameter
  );
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

std::shared_ptr<Text> EagleTypeConverter::convertSchematicText(
    const parseagle::Text& t) {
  return std::make_shared<Text>(
      Uuid::createRandom(),  // UUID
      convertLayer(t.getLayer()),  // Layer
      convertTextValue(t.getValue()),  // Text
      convertPoint(t.getPosition()),  // Position
      convertAngle(t.getRotation().getAngle()),  // Rotation
      PositiveLength(2500000),  // Height
      Alignment(HAlign::left(), VAlign::bottom())  // Alignment
  );
}

std::shared_ptr<StrokeText> EagleTypeConverter::convertBoardText(
    const parseagle::Text& t) {
  return std::make_shared<StrokeText>(
      Uuid::createRandom(),  // UUID
      convertLayer(t.getLayer()),  // Layer
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

std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad> >
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

std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad> >
    EagleTypeConverter::convertSmtPad(const parseagle::SmtPad& p) {
  Uuid uuid = Uuid::createRandom();
  const Layer& layer = convertLayer(p.getLayer());
  FootprintPad::ComponentSide side;
  if (layer == Layer::topCopper()) {
    side = FootprintPad::ComponentSide::Top;
  } else if (layer == Layer::botCopper()) {
    side = FootprintPad::ComponentSide::Bottom;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid pad layer: %1").arg(layer.getNameTr()));
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb
