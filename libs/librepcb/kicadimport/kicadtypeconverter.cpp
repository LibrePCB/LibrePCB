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
#include "kicadtypeconverter.h"

#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/geometry/text.h>
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/packagepad.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/clipperhelpers.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/tangentpathjoiner.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace kicadimport {

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ElementName KiCadTypeConverter::convertElementName(const QString& s) {
  return ElementName(cleanElementName(s));
}

QString KiCadTypeConverter::convertElementDescription(
    const FilePath& libFp, const QString& elemName,
    const QList<KiCadProperty>& props) {
  QString s;
  if (auto p = findProperty(props, "description")) {
    s = p->value;
  }
  s = s.trimmed();
  if (!s.isEmpty()) {
    s += "\n\n";
  }
  s += "Imported from KiCad (" % libFp.getBasename() % ":" % elemName % ").";
  return s;
}

QString KiCadTypeConverter::convertElementKeywords(
    const QString& commonKeywords, const QList<KiCadProperty>& props) {
  QString ret = commonKeywords;
  if (auto prop = findProperty(props, "ki_keywords")) {
    foreach (const QString& keyword, prop->value.split(' ')) {
      if (!keyword.trimmed().isEmpty()) {
        ret += "," % keyword.trimmed();
      }
    }
  }
  return ret;
}

ResourceList KiCadTypeConverter::convertResources(
    const QList<KiCadProperty>& props) {
  ResourceList ret;
  if (auto prop = findProperty(props, "datasheet")) {
    const QUrl url = prop->value.trimmed();
    if (url.isValid() && (prop->value.length() > 1)) {  // Ignore "~"
      ret.append(std::make_shared<Resource>(ElementName("Datasheet"),
                                            "application/pdf", url));
    }
  }
  return ret;
}

Point KiCadTypeConverter::convertSymbolPoint(const QPointF& p) {
  return Point::fromMm(p.x(), p.y());
}

Point KiCadTypeConverter::convertFootprintPoint(const QPointF& p) {
  return Point::fromMm(p.x(), -p.y());
}

Angle KiCadTypeConverter::convertArc(const Point& start, const Point& mid,
                                     const Point& end) {
  Angle angle = Toolbox::arcAngleFrom3Points(start, mid, end);
  if ((angle.mappedTo0_360deg() % Angle::deg45()) < Angle::fromDeg(0.1)) {
    angle.round(Angle::deg45());
  }
  return angle;
}

UnsignedLength KiCadTypeConverter::convertSymbolStrokeWidth(qreal width) {
  if (width <= 0) {
    return UnsignedLength(Length::fromMil(6));  // Default KiCad width.
  } else {
    return UnsignedLength(Length::fromMm(width));  // can throw
  }
}

PositiveLength KiCadTypeConverter::convertSymbolTextHeight(qreal height) {
  return PositiveLength(Length::fromMm(height * 2.5 / 1.27));
}

UnsignedLength KiCadTypeConverter::convertFootprintStrokeWidth(
    qreal width, const Layer& layer) {
  if (layer.isBoardEdge() || layer.getPolygonsRepresentAreas()) {
    return UnsignedLength(0);
  } else {
    return UnsignedLength(Length::fromMm(width));  // can throw
  }
}

std::shared_ptr<Polygon> KiCadTypeConverter::convertSymbolArc(
    const KiCadSymbolArc& a) {
  const bool fill = a.fillType == KiCadSymbolFillType::Outline;
  const bool grabArea = a.fillType == KiCadSymbolFillType::Background;
  const Point pA = convertSymbolPoint(a.start);
  const Point pM = convertSymbolPoint(a.mid);
  const Point pB = convertSymbolPoint(a.end);
  Path path = Path::line(pA, pB, convertArc(pA, pM, pB));
  if (fill) {
    path.close();  // KiCad fills even if not closed, but LibrePCB doesn't.
  }
  return std::make_shared<Polygon>(
      Uuid::createRandom(), Layer::symbolOutlines(),
      convertSymbolStrokeWidth(a.strokeWidth), fill, grabArea, path);
}

std::shared_ptr<Circle> KiCadTypeConverter::convertSymbolCircle(
    const KiCadSymbolCircle& c) {
  const bool fill = c.fillType == KiCadSymbolFillType::Outline;
  const bool grabArea = c.fillType == KiCadSymbolFillType::Background;
  return std::make_shared<Circle>(Uuid::createRandom(), Layer::symbolOutlines(),
                                  convertSymbolStrokeWidth(c.strokeWidth), fill,
                                  grabArea, convertSymbolPoint(c.center),
                                  PositiveLength(Length::fromMm(c.radius) * 2));
}

std::shared_ptr<Polygon> KiCadTypeConverter::convertSymbolRectangle(
    const KiCadSymbolRectangle& r) {
  const bool fill = r.fillType == KiCadSymbolFillType::Outline;
  const bool grabArea = r.fillType == KiCadSymbolFillType::Background;
  const Path path =
      Path::rect(convertSymbolPoint(r.start), convertSymbolPoint(r.end));
  return std::make_shared<Polygon>(
      Uuid::createRandom(), Layer::symbolOutlines(),
      convertSymbolStrokeWidth(r.strokeWidth), fill, grabArea, path);
}

std::shared_ptr<Polygon> KiCadTypeConverter::convertSymbolPolyline(
    const KiCadSymbolPolyline& p) {
  const bool fill = p.fillType == KiCadSymbolFillType::Outline;
  const bool grabArea = p.fillType == KiCadSymbolFillType::Background;
  Path path;
  for (const QPointF& pos : p.coordinates) {
    path.addVertex(convertSymbolPoint(pos));
  }
  if (path.getVertices().count() < 2) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Polygon with less than 2 vertices.");
  }
  if (fill) {
    path.close();  // KiCad fills even if not closed, but LibrePCB doesn't.
  }
  return std::make_shared<Polygon>(
      Uuid::createRandom(), Layer::symbolOutlines(),
      convertSymbolStrokeWidth(p.strokeWidth), fill, grabArea, path);
}

std::shared_ptr<Text> KiCadTypeConverter::convertSymbolText(
    const KiCadSymbolText& t) {
  // Fix "keep upright".
  Angle rotation = Angle::fromDeg(t.rotation);
  if ((rotation.mappedTo0_360deg() > Angle::deg90()) &&
      (rotation.mappedTo0_360deg() <= Angle::deg270())) {
    rotation += Angle::deg180();
  }

  // Note: These are probably pure graphical text (not NAME or VALUE) so it
  // makes sense to lock them?
  return std::make_shared<Text>(Uuid::createRandom(), Layer::symbolOutlines(),
                                t.text, convertFootprintPoint(t.position),
                                rotation,
                                convertSymbolTextHeight(t.fontSize.height()),
                                Alignment::fromQt(t.alignment), true);
}

std::shared_ptr<Text> KiCadTypeConverter::convertSymbolPropertyToText(
    const KiCadProperty& p) {
  const Layer* layer = nullptr;
  QString text;
  if ((p.key.toLower() == "reference") && (!p.hide)) {
    layer = &Layer::symbolNames();
    text = "{{NAME}}";
  } else if ((p.key.toLower() == "value") && (!p.hide)) {
    layer = &Layer::symbolValues();
    text = "{{VALUE}}";
  } else {
    return nullptr;
  }

  // Skip default-generated properties which are not displayed in KiCad.
  if (p.value.isEmpty() && p.position.isNull()) {
    return nullptr;
  }

  // Fix "keep upright".
  Angle rotation = Angle::fromDeg(p.rotation);
  if ((!p.unlocked) && (rotation.mappedTo0_360deg() > Angle::deg90()) &&
      (rotation.mappedTo0_360deg() <= Angle::deg270())) {
    rotation += Angle::deg180();
  }

  return std::make_shared<Text>(Uuid::createRandom(), *layer, text,
                                convertSymbolPoint(p.position), rotation,
                                convertSymbolTextHeight(p.fontSize.height()),
                                Alignment::fromQt(p.alignment), false);
}

QList<std::pair<QString, QStringList>>
    KiCadTypeConverter::convertSymbolPinNames(
        const QList<KiCadSymbolPin>& pins) {
  // Merge pins with identical name and position into a single pin.
  struct Pin {
    QString name;
    QStringList numbers;
    QPointF position;
  };
  QList<Pin> mergedPins;
  for (const KiCadSymbolPin& pin : pins) {
    Pin* existingPin = nullptr;
    for (Pin& mergedPin : mergedPins) {
      if ((mergedPin.name == pin.name) &&
          (mergedPin.position == pin.position)) {
        existingPin = &mergedPin;
        break;
      }
    }
    const QStringList numbers =
        pin.number.isEmpty() ? QStringList() : QStringList{pin.number};
    if (existingPin) {
      // There's already a pin at the same position -> just append pin number.
      existingPin->numbers.append(numbers);
      mergedPins.append(Pin());  // Ignore pin.
    } else {
      mergedPins.append(Pin{pin.name, numbers, pin.position});
    }
  }
  Q_ASSERT(mergedPins.count() == pins.count());

  // Determine new names.
  QStringList names;
  QSet<QString> usedPinNames;
  for (const Pin& pin : mergedPins) {
    QString name = pin.name;
    if (!name.isEmpty()) {
      name = *convertSymbolPinName(pin.name, pin.numbers);
      usedPinNames.insert(name);
    }
    names.append(name);
  }
  Q_ASSERT(names.count() == pins.count());

  // Make names unique.
  QList<std::pair<QString, QStringList>> ret;
  for (int i = 0; i < mergedPins.count(); ++i) {
    QString name = names.at(i);
    if (!name.isEmpty()) {
      if (names.count(name) > 1) {
        int number = 1;
        do {
          name = names.at(i) % "_" % QString::number(number++);
        } while (usedPinNames.contains(name));
      }
      usedPinNames.insert(name);
    }
    ret.append(std::make_pair(name, mergedPins.value(i).numbers));
  }
  Q_ASSERT(ret.count() == pins.count());
  return ret;
}

CircuitIdentifier KiCadTypeConverter::convertSymbolPinName(
    const QString& name, const QStringList& numbers) {
  QString out = name;
  if (out.isEmpty() || (out == "~")) {
    out = numbers.join(',');
  }
  return convertCircuitIdentifier(out);
}

CircuitIdentifier KiCadTypeConverter::convertCircuitIdentifier(
    const QString& text) {
  // Parse KiCad markup.
  struct Char {
    QChar ch;
    bool invert;
  };
  QList<Char> chars;
  bool inverted = false;
  for (int i = 0; i < text.length(); ++i) {
    if (text.mid(i, 2) == "~{") {
      inverted = true;
      ++i;
    } else if ((text.at(i) == '}') && inverted) {
      inverted = false;
    } else {
      chars.append(Char{text.at(i), inverted});
    }
  }

  // Convert to LibrePCB markup.
  QString out;
  inverted = false;
  for (int i = 0; i < chars.count(); ++i) {
    if ((!inverted) && chars.at(i).invert) {
      // Switch on inversion.
      if (chars.at(i).ch == '/') {
        out.append("!");
      }
      out.append("!");
      inverted = true;
    } else if (inverted && (!chars.at(i).invert)) {
      // Switch off inversion.
      if (chars.at(i).ch != '/') {
        out.append("!");
      }
      inverted = false;
    } else if (inverted && (chars.at(i).ch == '/')) {
      // Keep inversion on even though a slash follows.
      out.append("!");
    }
    out.append(chars.at(i).ch);
  }

  // Remove invalid characters.
  out = cleanCircuitIdentifier(out);
  if (out.isEmpty()) {
    out = "UNNAMED";
  }
  return CircuitIdentifier(out);
}

std::shared_ptr<SymbolPin> KiCadTypeConverter::convertSymbolPin(
    const KiCadSymbolPin& p, const QString& name, qreal pinNamesOffset) {
  const UnsignedLength length(Length::fromMm(p.length));
  return std::make_shared<SymbolPin>(
      Uuid::createRandom(), CircuitIdentifier(name),
      convertSymbolPoint(p.position), length, Angle::fromDeg(p.rotation),
      Point(length + Length::fromMm(pinNamesOffset), 0), Angle::deg0(),
      SymbolPin::getDefaultNameHeight(), SymbolPin::getDefaultNameAlignment());
}

const Layer& KiCadTypeConverter::convertFootprintGeometryLayer(
    const KiCadLayer& l) {
  switch (l) {
    case KiCadLayer::FrontAdhesion:
      return Layer::topGlue();
    case KiCadLayer::FrontCopper:
      return Layer::topCopper();
    case KiCadLayer::FrontCourtyard:
      return Layer::topCourtyard();
    case KiCadLayer::FrontFabrication:
      return Layer::topDocumentation();
    case KiCadLayer::FrontPaste:
      return Layer::topSolderPaste();
    case KiCadLayer::FrontSilkscreen:
      return Layer::topLegend();
    case KiCadLayer::FrontSolderMask:
      return Layer::topStopMask();

    case KiCadLayer::BackAdhesion:
      return Layer::botGlue();
    case KiCadLayer::BackCopper:
      return Layer::botCopper();
    case KiCadLayer::BackCourtyard:
      return Layer::botCourtyard();
    case KiCadLayer::BackFabrication:
      return Layer::botDocumentation();
    case KiCadLayer::BackPaste:
      return Layer::botSolderPaste();
    case KiCadLayer::BackSilkscreen:
      return Layer::botLegend();
    case KiCadLayer::BackSolderMask:
      return Layer::botStopMask();

    case KiCadLayer::BoardOutline:
      return Layer::boardOutlines();
    case KiCadLayer::UserComment:
      return Layer::boardComments();
    case KiCadLayer::UserDrawing:
      return Layer::boardDocumentation();

    default:
      break;
  }

  throw RuntimeError(__FILE__, __LINE__,
                     QString("Unsupported footprint geometry layer %1.")
                         .arg(static_cast<int>(l)));
}

KiCadTypeConverter::Line KiCadTypeConverter::convertFootprintLine(
    const KiCadFootprintLine& l) {
  const Layer& layer = convertFootprintGeometryLayer(l.layer);
  return Line{
      &layer,
      convertFootprintStrokeWidth(l.strokeWidth, layer),
      convertFootprintPoint(l.start),
      convertFootprintPoint(l.end),
      Angle::deg0(),
  };
}

KiCadTypeConverter::Line KiCadTypeConverter::convertFootprintArc(
    const KiCadFootprintArc& a) {
  const Point pA = convertFootprintPoint(a.start);
  const Point pM = convertFootprintPoint(a.mid);
  const Point pB = convertFootprintPoint(a.end);
  const Layer& layer = convertFootprintGeometryLayer(a.layer);
  return Line{
      &layer,
      convertFootprintStrokeWidth(a.strokeWidth, layer),
      pA,
      pB,
      convertArc(pA, pM, pB),
  };
}

QList<KiCadTypeConverter::LineGroup>
    KiCadTypeConverter::groupLinesByLayerAndWidth(const QList<Line>& lines) {
  QMap<std::pair<const Layer*, UnsignedLength>, QList<Line>> map;
  for (const Line& line : lines) {
    map[std::make_pair(line.layer, line.width)].append(line);
  }
  QList<LineGroup> groups;
  for (auto it = map.begin(); it != map.end(); it++) {
    LineGroup group{it.key().first, it.key().second, {}};
    for (const Line& line : it.value()) {
      group.paths.append(Path::line(line.start, line.end, line.angle));
    }
    groups.append(group);
  }
  return groups;
}

std::shared_ptr<Circle> KiCadTypeConverter::convertFootprintCircle(
    const KiCadFootprintCircle& c) {
  const Point center = convertFootprintPoint(c.center);
  const Point end = convertFootprintPoint(c.end);
  Length diameter(*(end - center).getLength() * 2);
  const Layer& layer = convertFootprintGeometryLayer(c.layer);
  UnsignedLength lineWidth = convertFootprintStrokeWidth(c.strokeWidth, layer);
  bool fill = c.fillType == KiCadFootprintFillType::Solid;
  const bool grabArea = false;
  if (diameter <= *lineWidth) {
    diameter += *lineWidth;
    lineWidth = UnsignedLength(0);
    fill = true;
  }
  return std::make_shared<Circle>(Uuid::createRandom(), layer, lineWidth, fill,
                                  grabArea, center, PositiveLength(diameter));
}

std::shared_ptr<Polygon> KiCadTypeConverter::convertFootprintRectangle(
    const KiCadFootprintRectangle& r) {
  const Layer& layer = convertFootprintGeometryLayer(r.layer);
  const bool fill = r.fillType == KiCadFootprintFillType::Solid;
  const bool grabArea = false;
  const Path path =
      Path::rect(convertFootprintPoint(r.start), convertFootprintPoint(r.end));
  return std::make_shared<Polygon>(
      Uuid::createRandom(), layer,
      convertFootprintStrokeWidth(r.strokeWidth, layer), fill, grabArea, path);
}

std::shared_ptr<Polygon> KiCadTypeConverter::convertFootprintPolygon(
    const KiCadFootprintPolygon& p) {
  const Layer& layer = convertFootprintGeometryLayer(p.layer);
  const bool fill = p.fillType == KiCadFootprintFillType::Solid;
  const bool grabArea = false;
  Path path;
  for (const QPointF& pos : p.coordinates) {
    path.addVertex(convertFootprintPoint(pos));
  }
  if (path.getVertices().count() < 2) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Polygon with less than 2 vertices.");
  }
  path.close();  // KiCad polygons are always closed.
  return std::make_shared<Polygon>(
      Uuid::createRandom(), layer,
      convertFootprintStrokeWidth(p.strokeWidth, layer), fill, grabArea, path);
}

std::shared_ptr<Zone> KiCadTypeConverter::convertFootprintZone(
    const KiCadZone& z, MessageLogger& log) {
  Zone::Layers layers;
  layers.setFlag(Zone::Layer::Top,
                 z.layers.contains(KiCadLayer::FrontCopper) ||
                     z.layers.contains(KiCadLayer::FrontAndBackCopper) ||
                     z.layers.contains(KiCadLayer::AllCopper));
  layers.setFlag(Zone::Layer::Inner,
                 z.layers.contains(KiCadLayer::InnerCopper1) ||
                     z.layers.contains(KiCadLayer::AllCopper));
  layers.setFlag(Zone::Layer::Bottom,
                 z.layers.contains(KiCadLayer::BackCopper) ||
                     z.layers.contains(KiCadLayer::FrontAndBackCopper) ||
                     z.layers.contains(KiCadLayer::AllCopper));
  if (layers == Zone::Layers()) {
    log.warning("Zone without any layers, will be discarded.");
    return nullptr;
  }

  Zone::Rules rules;
  rules.setFlag(Zone::Rule::NoCopper,
                z.keepOutTracks || z.keepOutVias || z.keepOutPads);
  rules.setFlag(Zone::Rule::NoPlanes, z.keepOutCopperPour);
  rules.setFlag(Zone::Rule::NoExposure, false);
  rules.setFlag(Zone::Rule::NoDevices, z.keepOutFootprints);
  if ((!z.keepOutTracks) && (z.keepOutVias || z.keepOutPads)) {
    log.warning(
        "Via- or pad-keepout in zone is not supported, will be ignored.");
  }
  if (rules == Zone::Rules()) {
    log.warning("Zone without any rules, will be discarded.");
    return nullptr;
  }

  Path outline;
  for (const QPointF& p : z.polygon) {
    outline.addVertex(convertFootprintPoint(p));
  }
  outline.open();
  outline.clean();
  if (outline.getVertices().count() < 3) {
    log.warning("Invalid zone outline, will be discarded.");
    return nullptr;
  }

  return std::make_shared<Zone>(Uuid::createRandom(), layers, rules, outline);
}

std::shared_ptr<StrokeText> KiCadTypeConverter::convertFootprintText(
    const KiCadFootprintText& t) {
  // Discard value on documentation layer as we don't use that.
  if (t.text == "${REFERENCE}") {
    return nullptr;
  }

  // Fix "keep upright".
  Angle rotation = Angle::fromDeg(t.rotation);
  if ((!t.unlocked) && (rotation.mappedTo0_360deg() > Angle::deg90()) &&
      (rotation.mappedTo0_360deg() <= Angle::deg270())) {
    rotation += Angle::deg180();
  }

  const Layer& layer = convertFootprintGeometryLayer(t.layer);
  return std::make_shared<StrokeText>(
      Uuid::createRandom(), layer, t.text, convertFootprintPoint(t.position),
      rotation, PositiveLength(Length::fromMm(t.fontSize.height())),
      UnsignedLength(Length::fromMm(t.fontThickness)), StrokeTextSpacing(),
      StrokeTextSpacing(), Alignment::fromQt(t.alignment), t.mirror,
      !t.unlocked);
}

std::shared_ptr<StrokeText> KiCadTypeConverter::convertFootprintPropertyToText(
    const KiCadProperty& p) {
  const Layer* layer = nullptr;
  QString text;
  if ((p.key.toLower() == "reference") && (p.value == "REF**") && (!p.hide)) {
    layer = &Layer::topNames();
    text = "{{NAME}}";
  } else if ((p.key.toLower() == "value") && (!p.hide)) {
    layer = &Layer::topValues();
    text = "{{VALUE}}";
  } else {
    return nullptr;
  }

  // Skip default-generated properties which are not displayed in KiCad.
  if (p.value.isEmpty() && p.position.isNull()) {
    return nullptr;
  }

  // Fix "keep upright".
  Angle rotation = Angle::fromDeg(p.rotation);
  if ((!p.unlocked) && (rotation.mappedTo0_360deg() > Angle::deg90()) &&
      (rotation.mappedTo0_360deg() <= Angle::deg270())) {
    rotation += Angle::deg180();
  }

  return std::make_shared<StrokeText>(
      Uuid::createRandom(), *layer, text, convertFootprintPoint(p.position),
      rotation, PositiveLength(Length::fromMm(p.fontSize.height())),
      UnsignedLength(Length::fromMm(p.fontThickness)), StrokeTextSpacing(),
      StrokeTextSpacing(), Alignment::fromQt(p.alignment), p.mirror,
      !p.unlocked);
}

KiCadTypeConverter::PadReplacements KiCadTypeConverter::convertPad(
    const KiCadFootprintPad& p, qreal fptSolderMaskMargin,
    qreal fptSolderPasteMargin, qreal fptSolderPasteRatio, qreal fptClearance,
    MessageLogger& log) {
  // Convert layers.
  const QMap<KiCadLayer, const Layer*> layerMap = {
      {KiCadLayer::FrontCopper, &Layer::topCopper()},
      {KiCadLayer::FrontAdhesion, &Layer::topGlue()},
      {KiCadLayer::FrontPaste, &Layer::topSolderPaste()},
      {KiCadLayer::FrontSilkscreen, &Layer::topLegend()},
      {KiCadLayer::FrontSolderMask, &Layer::topStopMask()},
      {KiCadLayer::BackCopper, &Layer::botCopper()},
      {KiCadLayer::BackAdhesion, &Layer::botGlue()},
      {KiCadLayer::BackPaste, &Layer::botSolderPaste()},
      {KiCadLayer::BackSilkscreen, &Layer::botLegend()},
      {KiCadLayer::BackSolderMask, &Layer::botStopMask()},
  };
  QSet<const Layer*> layers;
  QSet<const Layer*> handledLayers;
  for (const KiCadLayer& kiLayer : p.layers) {
    if (const Layer* layer = layerMap.value(kiLayer, nullptr)) {
      layers.insert(layer);
    } else if ((kiLayer == KiCadLayer::AllCopper) ||
               (kiLayer == KiCadLayer::FrontAndBackCopper)) {
      layers.insert(&Layer::topCopper());
      layers.insert(&Layer::botCopper());
    } else if (kiLayer == KiCadLayer::AllSolderMask) {
      layers.insert(&Layer::topStopMask());
      layers.insert(&Layer::botStopMask());
    } else if ((kiLayer == KiCadLayer::AllSilkscreen) ||
               (kiLayer == KiCadLayer::FrontSilkscreen ||
                kiLayer == KiCadLayer::BackSilkscreen)) {
      log.warning("Silkscreen enabled on pad, don't know what to do with it.");
    } else {
      log.warning(QString("Unsupported layer %1 enabled on pad.")
                      .arg(static_cast<int>(kiLayer)));
    }
  }

  // Detect & convert pad shape.
  PositiveLength width(Length::fromMm(p.size.width()));
  PositiveLength height(Length::fromMm(p.size.height()));
  UnsignedLimitedRatio radius(Ratio::fromPercent(0));
  Pad::Shape shape = Pad::Shape::RoundedRect;
  Path customShapeOutline;
  Path actualShapeOutline;  // Used for creating polygon.
  if ((p.shape == KiCadPadShape::Circle) || (p.shape == KiCadPadShape::Oval)) {
    // Circle or obround.
    shape = Pad::Shape::RoundedRect;
    radius = UnsignedLimitedRatio(Ratio::fromPercent(100));
    actualShapeOutline = Path::obround(width, height);
  } else if ((p.shape == KiCadPadShape::Rect) ||
             ((p.shape == KiCadPadShape::RoundRect) &&
              (p.roundRectRRatio == 0)) ||
             ((p.shape == KiCadPadShape::Trapezoid) &&
              (p.rectDelta.isNull()))) {
    const Length chmferSize = std::min(width, height)->scaled(p.chamferRatio);
    if ((chmferSize > 0) && (!p.chamferEdges.isEmpty())) {
      // Chamfered rect.
      shape = Pad::Shape::Custom;
      customShapeOutline =
          Path::chamferedRect(width, height, UnsignedLength(chmferSize),
                              p.chamferEdges.contains(KiCadEdge::TopLeft),
                              p.chamferEdges.contains(KiCadEdge::TopRight),
                              p.chamferEdges.contains(KiCadEdge::BottomLeft),
                              p.chamferEdges.contains(KiCadEdge::BottomRight));
      actualShapeOutline = customShapeOutline;
    } else {
      // Plain rect.
      shape = Pad::Shape::RoundedRect;
      radius = UnsignedLimitedRatio(Ratio::fromPercent(0));
      actualShapeOutline = Path::centeredRect(width, height);
    }
  } else if (p.shape == KiCadPadShape::RoundRect) {
    // Rounded rect.
    shape = Pad::Shape::RoundedRect;
    radius = UnsignedLimitedRatio(Ratio::fromNormalized(
        qBound(qreal(0), p.roundRectRRatio * 2, qreal(1))));
    actualShapeOutline = Path::centeredRect(
        width, height,
        UnsignedLength(std::min(width, height)->scaled(p.roundRectRRatio)));
    if ((p.chamferRatio > 0) && (!p.chamferEdges.isEmpty())) {
      log.warning(
          "Pads with mixed rounded and chamfered edges are not supported yet.");
    }
  } else if (p.shape == KiCadPadShape::Trapezoid) {
    // Trapezoidal.
    shape = Pad::Shape::Custom;
    customShapeOutline =
        Path::trapezoid(width, height, -Length::fromMm(p.rectDelta.height()),
                        -Length::fromMm(p.rectDelta.width()));
    actualShapeOutline = customShapeOutline;
  } else if (p.shape == KiCadPadShape::Custom) {
    // Custom outline.
    Clipper2Lib::Paths64 paths;
    auto addToPaths = [&paths](const Path& path, const Length& width) {
      Clipper2Lib::Paths64 tmp{
          ClipperHelpers::convert(path, maxArcTolerance())};
      if ((width / 2) > 0) {
        ClipperHelpers::offset(tmp, width / 2, maxArcTolerance(),
                               Clipper2Lib::JoinType::Round);
      }
      ClipperHelpers::unite(paths, tmp, Clipper2Lib::FillRule::EvenOdd,
                            Clipper2Lib::FillRule::NonZero);
    };
    for (const KiCadGraphicalLine& line : p.graphicalLines) {
      const Point start = convertFootprintPoint(line.start);
      const Point end = convertFootprintPoint(line.end);
      addToPaths(
          Path::obround(start, end, PositiveLength(Length::fromMm(line.width))),
          Length(0));
    }
    for (const KiCadGraphicalArc& arc : p.graphicalArcs) {
      const Point start = convertFootprintPoint(arc.start);
      const Point mid = convertFootprintPoint(arc.mid);
      const Point end = convertFootprintPoint(arc.end);
      const Angle angle = convertArc(start, mid, end);
      addToPaths(Path::arcObround(start, end, angle,
                                  PositiveLength(Length::fromMm(arc.width))),
                 Length(0));
    }
    for (const KiCadGraphicalCircle& circle : p.graphicalCircles) {
      const Point center = convertFootprintPoint(circle.center);
      const Point end = convertFootprintPoint(circle.end);
      const PositiveLength diameter((end - center).getLength() * 2);
      const Length width = Length::fromMm(circle.width);
      const Length outerDiameter = diameter + width;
      const Length holeDiameter = diameter - width;
      if ((outerDiameter > 0) && ((circle.fill) || (holeDiameter <= 0))) {
        addToPaths(
            Path::circle(PositiveLength(outerDiameter)).translated(center),
            Length(0));
      } else if ((outerDiameter > holeDiameter) && (holeDiameter > 0)) {
        addToPaths(Path::donut(PositiveLength(outerDiameter),
                               PositiveLength(holeDiameter))
                       .translated(center),
                   Length(0));
      } else {
        log.warning("Strange circle in custom pad shape ignored.");
      }
    }
    for (const KiCadGraphicalPolygon& polygon : p.graphicalPolygons) {
      Path outline;
      for (const QPointF& coordinate : polygon.coordinates) {
        outline.addVertex(convertFootprintPoint(coordinate));
      }
      addToPaths(outline, Length::fromMm(polygon.width));
    }
    // Add pad anchor.
    if (p.customPadAnchor == KiCadCustomPadAnchor::Rect) {
      shape = Pad::Shape::RoundedRect;  // Fallback.
      radius = UnsignedLimitedRatio(Ratio::fromPercent(0));  // Fallback.
      actualShapeOutline = Path::centeredRect(width, height);  // Fallback.
      if (!paths.empty()) {
        addToPaths(Path::centeredRect(width, height), Length(0));
      }
    } else {
      if (p.customPadAnchor != KiCadCustomPadAnchor::Circle) {
        log.critical(
            QString("Invalid custom pad anchor %1, using circular shape.")
                .arg(static_cast<int>(p.customPadAnchor)));
      }
      shape = Pad::Shape::RoundedRect;  // Fallback.
      radius = UnsignedLimitedRatio(Ratio::fromPercent(100));  // Fallback.
      actualShapeOutline = Path::obround(width, height);  // Fallback.
      if (!paths.empty()) {
        addToPaths(Path::obround(width, height), Length(0));
      }
    }
    std::unique_ptr<Clipper2Lib::PolyTree64> tree =
        ClipperHelpers::uniteToTree(paths, {}, Clipper2Lib::FillRule::EvenOdd,
                                    Clipper2Lib::FillRule::EvenOdd);
    paths = ClipperHelpers::flattenTree(*tree);
    if (!paths.empty()) {
      if (paths.size() > 1) {
        log.critical(
            "Custom pad shape consists of multiple separated primitives, "
            "considering only one of them.");
      }
      shape = Pad::Shape::Custom;
      customShapeOutline = ClipperHelpers::convert(paths.front());
      actualShapeOutline = customShapeOutline;
    } else {
      log.critical("Custom pad shape does not have a custom shape set.");
    }
  } else {
    log.critical(
        QString("Unsupported pad shape %1, using circular shape instead.")
            .arg(static_cast<int>(p.shape)));
    shape = Pad::Shape::RoundedRect;
    radius = UnsignedLimitedRatio(Ratio::fromPercent(100));
    actualShapeOutline = Path::obround(width, height);
  }
  customShapeOutline.open();  // Considered as closed by LibrePCB.
  customShapeOutline.clean();
  actualShapeOutline.close();  // Must be closed for polygons.
  actualShapeOutline.clean();
  actualShapeOutline.rotate(Angle::fromDeg(p.rotation));
  actualShapeOutline.translate(convertFootprintPoint(p.position));
  if (actualShapeOutline.getVertices().count() < 2) {
    log.critical("Pad shape detection failed.");
  }

  // Convert drill.
  std::optional<PositiveLength> drillDiameter;
  std::optional<NonEmptyPath> drillPath;
  const Length drillWidth = Length::fromMm(std::max(p.drill.width(), qreal(0)));
  const Length drillHeight =
      Length::fromMm(std::max(p.drill.height(), qreal(0)));
  if ((drillWidth > 0) && (drillHeight > 0)) {
    drillDiameter = PositiveLength(std::min(drillWidth, drillHeight));
    if (drillWidth != drillHeight) {
      const Length dx = drillWidth - *drillDiameter;
      const Length dy = drillHeight - *drillDiameter;
      drillPath = NonEmptyPath(
          Path::line(Point(-dx / 2, dy / 2), Point(dx / 2, -dy / 2)));
    } else {
      drillPath = makeNonEmptyPath(Point(0, 0));
    }
  } else if ((p.type == KiCadPadType::ThruHole) ||
             (p.type == KiCadPadType::NpThruHole)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Through-hole pad has no valid drill set.");
  }
  if (((drillWidth > 0) || (drillHeight > 0)) &&
      (p.type != KiCadPadType::ThruHole) &&
      (p.type != KiCadPadType::NpThruHole)) {
    log.warning("SMD pad has a drill diameter specified, it will be ignored.");
    drillDiameter = std::nullopt;
    drillPath = std::nullopt;
  }

  // Determine stop mask config.
  bool hasStopMask = false;
  if ((drillDiameter) && (drillPath) &&
      (layers.contains(&Layer::topStopMask()) ||
       layers.contains(&Layer::botStopMask()))) {
    hasStopMask = true;
    if ((!layers.contains(&Layer::topStopMask())) ||
        (!layers.contains(&Layer::botStopMask()))) {
      log.warning("THT pad with stop mask only on one side is not supported.");
    }
  } else if ((layers.contains(&Layer::topCopper()) &&
              (!layers.contains(&Layer::botCopper()))) &&
             layers.contains(&Layer::topStopMask())) {
    hasStopMask = true;
    if (layers.contains(&Layer::botStopMask())) {
      log.warning("SMD pad with stop mask on both sides is not supported.");
    }
  } else if ((layers.contains(&Layer::botCopper()) &&
              (!layers.contains(&Layer::topCopper()))) &&
             layers.contains(&Layer::botStopMask())) {
    hasStopMask = true;
    if (layers.contains(&Layer::topStopMask())) {
      log.warning("SMD pad with stop mask on both sides is not supported.");
    }
  }
  const qreal solderMaskMargin =
      (p.solderMaskMargin != 0) ? p.solderMaskMargin : fptSolderMaskMargin;
  const MaskConfig stopMaskConfig = (!hasStopMask)
      ? MaskConfig::off()
      : ((solderMaskMargin != 0)
             ? MaskConfig::manual(Length::fromMm(solderMaskMargin))
             : MaskConfig::automatic());

  // Determine copper clearance.
  const UnsignedLength copperClearance(
      Length::fromMm(std::max(p.clearance, fptClearance)));

  // Handle pad.
  PadReplacements result;
  if ((p.type != KiCadPadType::NpThruHole) &&
      (layers.contains(&Layer::topCopper()) ||
       layers.contains(&Layer::botCopper()))) {
    // Determine pad type/side.
    PadHoleList holes;
    Pad::ComponentSide cmpSide = Pad::ComponentSide::Top;
    if ((p.type == KiCadPadType::ThruHole) && (drillDiameter) && (drillPath) &&
        layers.contains(&Layer::topCopper()) &&
        layers.contains(&Layer::botCopper())) {
      // It's a THT pad.
      cmpSide = Pad::ComponentSide::Top;
      holes.append(std::make_shared<PadHole>(
          Uuid::createRandom(), *drillDiameter,
          NonEmptyPath(
              (*drillPath)->translated(-convertFootprintPoint(p.offset)))));
      handledLayers.insert(&Layer::topCopper());
      handledLayers.insert(&Layer::botCopper());
      handledLayers.insert(&Layer::topStopMask());
      handledLayers.insert(&Layer::botStopMask());
      handledLayers.insert(&Layer::topSolderPaste());
      handledLayers.insert(&Layer::botSolderPaste());
    } else if (layers.contains(&Layer::topCopper()) &&
               (!layers.contains(&Layer::botCopper()))) {
      // It's a top side pad.
      cmpSide = Pad::ComponentSide::Top;
      handledLayers.insert(&Layer::topCopper());
      handledLayers.insert(&Layer::topStopMask());
      handledLayers.insert(&Layer::topSolderPaste());
    } else if (layers.contains(&Layer::botCopper()) &&
               (!layers.contains(&Layer::topCopper()))) {
      // It's a bottom side pad.
      cmpSide = Pad::ComponentSide::Bottom;
      handledLayers.insert(&Layer::botCopper());
      handledLayers.insert(&Layer::botStopMask());
      handledLayers.insert(&Layer::botSolderPaste());
    } else {
      throw RuntimeError(__FILE__, __LINE__,
                         "Strange/unsupported pad configuration detected.");
    }

    // Determine solder paste config.
    bool hasSolderPaste = false;
    if ((!holes.isEmpty()) &&
        (layers.contains(&Layer::topSolderPaste()) ||
         layers.contains(&Layer::botSolderPaste()))) {
      hasSolderPaste = true;
      if ((!layers.contains(&Layer::topSolderPaste())) ||
          (!layers.contains(&Layer::botSolderPaste()))) {
        log.warning(
            "THT pad with solder paste only on one side is not supported.");
      }
    } else if ((cmpSide == Pad::ComponentSide::Top) &&
               layers.contains(&Layer::topSolderPaste())) {
      hasSolderPaste = true;
      if (layers.contains(&Layer::botSolderPaste())) {
        log.warning(
            "SMD pad with solder paste on both sides is not supported.");
      }
    } else if ((cmpSide == Pad::ComponentSide::Bottom) &&
               layers.contains(&Layer::botSolderPaste())) {
      hasSolderPaste = true;
      if (layers.contains(&Layer::topSolderPaste())) {
        log.warning(
            "SMD pad with solder paste on both sides is not supported.");
      }
    }
    const qreal solderPasteMargin =
        (p.solderPasteMargin != 0) ? p.solderPasteMargin : fptSolderPasteMargin;
    const qreal solderPasteRatio = (p.solderPasteMarginRatio != 0)
        ? p.solderPasteMarginRatio
        : fptSolderPasteRatio;
    const MaskConfig solderPasteConfig = (!hasSolderPaste)
        ? MaskConfig::off()
        : (((solderPasteMargin != 0) || (solderPasteRatio != 0))
               ? MaskConfig::manual(
                     -Length::fromMm(solderPasteMargin) -
                     std::min(width, height)->scaled(solderPasteRatio))
               : MaskConfig::automatic());

    // Determine pad function.
    Pad::Function function = Pad::Function::Unspecified;
    if (p.property == KiCadPadProperty::Bga) {
      function = Pad::Function::BgaPad;
    } else if (p.property == KiCadPadProperty::FiducialGlobal) {
      function = Pad::Function::GlobalFiducial;
    } else if (p.property == KiCadPadProperty::FiducialLocal) {
      function = Pad::Function::LocalFiducial;
    } else if (p.property == KiCadPadProperty::Testpoint) {
      function = Pad::Function::TestPad;
    } else if (p.property == KiCadPadProperty::Heatsink) {
      function = Pad::Function::ThermalPad;
    } else if (p.type == KiCadPadType::Connect) {
      function = Pad::Function::EdgeConnectorPad;
    }

    // Determine positioning.
    const Angle rotation = Angle::fromDeg(p.rotation);
    const Point position = convertFootprintPoint(p.position) +
        convertFootprintPoint(p.offset).rotated(rotation);

    // Create the pad.
    result.fptPad = std::make_shared<FootprintPad>(
        Uuid::createRandom(),  // UUID
        std::nullopt,  // Package pad UUID
        position,  // Position
        rotation,  // Rotation
        shape,  // Shape
        width,  // Width
        height,  // Height
        radius,  // Radius
        customShapeOutline,  // Custom shape outline
        stopMaskConfig,  // Stop mask
        solderPasteConfig,  // Solder paste
        copperClearance,  // Copper clearance
        cmpSide,  // Side
        function,  // Function
        holes  // Holes
    );
  }

  // Handle NPTH.
  if ((p.type == KiCadPadType::NpThruHole) && drillDiameter && drillPath) {
    handledLayers.insert(&Layer::topStopMask());
    handledLayers.insert(&Layer::botStopMask());
    result.hole = std::make_shared<Hole>(
        Uuid::createRandom(), *drillDiameter,
        NonEmptyPath((*drillPath)
                         ->rotated(Angle::fromDeg(p.rotation))
                         .translated(convertFootprintPoint(p.position))),
        stopMaskConfig);
    if (((width > drillWidth) || (height > drillHeight)) &&
        (layers.contains(&Layer::topCopper()) ||
         layers.contains(&Layer::botCopper()))) {
      log.critical(
          "NPTH with copper on top and/or bottom side is not supported.");
    }
    if ((copperClearance > 0) &&
        (layers.contains(&Layer::topCopper()) ||
         layers.contains(&Layer::botCopper()))) {
      log.critical(
          "Copper clearance on NPTH is not supported and will be ignored.");
    }
    handledLayers.insert(&Layer::topCopper());
    handledLayers.insert(&Layer::botCopper());
  }

  // Handle polygon. No idea why they call it a pad when meaning polygon.
  if ((p.type == KiCadPadType::Smd) && (!result.fptPad) &&
      (!actualShapeOutline.getVertices().isEmpty())) {
    // It's only a polygon, who knows why they call it pad.
    for (KiCadLayer kiLayer : p.layers) {
      if (const Layer* layer = layerMap.value(kiLayer, nullptr)) {
        result.polygons.append(std::make_shared<Polygon>(
            Uuid::createRandom(), *layer, UnsignedLength(0), true, false,
            actualShapeOutline));
        handledLayers.insert(layer);
      } else {
        log.critical(QString("SMD aperture with unsupported layer %1.")
                         .arg(static_cast<int>(p.layers.first())));
      }
    }
  }

  // Warn about unsupported pad.
  if ((!result.fptPad) && (!result.hole) && (result.polygons.isEmpty())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Could not convert pad '%1'.").arg(p.number));
  }

  // Warn about unhandled layers.
  foreach (const Layer* layer, layers - handledLayers) {
    log.critical(QString("Don't know what to do with layer '%1' on pad.")
                     .arg(layer->getNameTr()));
  }

  return result;
}

std::optional<KiCadProperty> KiCadTypeConverter::findProperty(
    const QList<KiCadProperty>& props, const QString& key) {
  for (const KiCadProperty& p : props) {
    if (p.key.toLower() == key.toLower()) {
      return p;
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
