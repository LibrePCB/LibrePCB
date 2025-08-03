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
#include "interactivehtmlbom.h"

#include "../exceptions.h"
#include "../utils/toolbox.h"
#include "../utils/transform.h"

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

template <>
std::unique_ptr<SExpression> serialize(
    const InteractiveHtmlBom::ViewMode& obj) {
  switch (obj) {
    case InteractiveHtmlBom::ViewMode::BomOnly:
      return SExpression::createToken("bom_only");
    case InteractiveHtmlBom::ViewMode::LeftRight:
      return SExpression::createToken("left_right");
    case InteractiveHtmlBom::ViewMode::TopBottom:
      return SExpression::createToken("top_bottom");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
InteractiveHtmlBom::ViewMode deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("bom_only")) {
    return InteractiveHtmlBom::ViewMode::BomOnly;
  } else if (str == QLatin1String("left_right")) {
    return InteractiveHtmlBom::ViewMode::LeftRight;
  } else if (str == QLatin1String("top_bottom")) {
    return InteractiveHtmlBom::ViewMode::TopBottom;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown interactive BOM view mode: '%1'").arg(str));
  }
}

template <>
std::unique_ptr<SExpression> serialize(
    const InteractiveHtmlBom::HighlightPin1Mode& obj) {
  switch (obj) {
    case InteractiveHtmlBom::HighlightPin1Mode::None:
      return SExpression::createToken("none");
    case InteractiveHtmlBom::HighlightPin1Mode::Selected:
      return SExpression::createToken("selected");
    case InteractiveHtmlBom::HighlightPin1Mode::All:
      return SExpression::createToken("all");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
InteractiveHtmlBom::HighlightPin1Mode deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("none")) {
    return InteractiveHtmlBom::HighlightPin1Mode::None;
  } else if (str == QLatin1String("selected")) {
    return InteractiveHtmlBom::HighlightPin1Mode::Selected;
  } else if (str == QLatin1String("all")) {
    return InteractiveHtmlBom::HighlightPin1Mode::All;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown interactive BOM pin1 mode: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Class InteractiveHtmlBom
 ******************************************************************************/

static RustHandle<rs::InteractiveHtmlBom> construct(
    const QString& title, const QString& company, const QString& revision,
    const QString& date, const Point& topLeft, const Point& bottomRight) {
  if (auto obj =
          rs::ffi_ibom_new(&title, &company, &revision, &date,
                           topLeft.getX().toMm(), -bottomRight.getY().toMm(),
                           bottomRight.getX().toMm(), -topLeft.getY().toMm())) {
    return RustHandle<rs::InteractiveHtmlBom>(*obj, &rs::ffi_ibom_delete);
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Failed to create InteractiveHtmlBom"));
  }
}

InteractiveHtmlBom::InteractiveHtmlBom(
    const QString& title, const QString& company, const QString& revision,
    const QString& date, const Point& topLeft, const Point& bottomRight)
  : mHandle(construct(title, company, revision, date, topLeft, bottomRight)) {
}

void InteractiveHtmlBom::setViewConfig(ViewMode mode,
                                       HighlightPin1Mode highlightPin1,
                                       bool dark) noexcept {
  static const QHash<ViewMode, rs::InteractiveHtmlBomViewMode> modeMap = {
      {ViewMode::BomOnly, rs::InteractiveHtmlBomViewMode::BomOnly},
      {ViewMode::LeftRight, rs::InteractiveHtmlBomViewMode::LeftRight},
      {ViewMode::TopBottom, rs::InteractiveHtmlBomViewMode::TopBottom},
  };
  static const QHash<HighlightPin1Mode, rs::InteractiveHtmlBomHighlightPin1Mode>
      pin1Map = {
          {HighlightPin1Mode::None,
           rs::InteractiveHtmlBomHighlightPin1Mode::None},
          {HighlightPin1Mode::Selected,
           rs::InteractiveHtmlBomHighlightPin1Mode::Selected},
          {HighlightPin1Mode::All,
           rs::InteractiveHtmlBomHighlightPin1Mode::All},
      };
  rs::ffi_ibom_set_view_config(*mHandle, modeMap[mode], pin1Map[highlightPin1],
                               dark);
}

void InteractiveHtmlBom::setBoardRotation(const Angle& angle,
                                          bool offsetBack) noexcept {
  rs::ffi_ibom_set_rotation(*mHandle, angle.toDeg(), offsetBack);
}

void InteractiveHtmlBom::setShowSilkscreen(bool show) noexcept {
  rs::ffi_ibom_set_show_silkscreen(*mHandle, show);
}

void InteractiveHtmlBom::setShowFabrication(bool show) noexcept {
  rs::ffi_ibom_set_show_fabrication(*mHandle, show);
}

void InteractiveHtmlBom::setShowPads(bool show) noexcept {
  rs::ffi_ibom_set_show_pads(*mHandle, show);
}

void InteractiveHtmlBom::setCheckBoxes(const QStringList& names) noexcept {
  rs::ffi_ibom_set_checkboxes(*mHandle, &names);
}

void InteractiveHtmlBom::setFields(const QStringList& fields) noexcept {
  rs::ffi_ibom_set_fields(*mHandle, &fields);
}

void InteractiveHtmlBom::addDrawing(DrawingKind kind, DrawingLayer layer,
                                    const Path& path,
                                    const UnsignedLength& width,
                                    bool filled) noexcept {
  static const QHash<DrawingKind, rs::InteractiveHtmlBomDrawingKind> kindMap = {
      {DrawingKind::Polygon, rs::InteractiveHtmlBomDrawingKind::Polygon},
      {DrawingKind::ReferenceText,
       rs::InteractiveHtmlBomDrawingKind::ReferenceText},
      {DrawingKind::ValueText, rs::InteractiveHtmlBomDrawingKind::ValueText},
  };
  static const QHash<DrawingLayer, rs::InteractiveHtmlBomDrawingLayer>
      layerMap = {
          {DrawingLayer::Edge, rs::InteractiveHtmlBomDrawingLayer::Edge},
          {DrawingLayer::SilkscreenFront,
           rs::InteractiveHtmlBomDrawingLayer::SilkscreenFront},
          {DrawingLayer::SilkscreenBack,
           rs::InteractiveHtmlBomDrawingLayer::SilkscreenBack},
          {DrawingLayer::FabricationFront,
           rs::InteractiveHtmlBomDrawingLayer::FabricationFront},
          {DrawingLayer::FabricationBack,
           rs::InteractiveHtmlBomDrawingLayer::FabricationBack},
      };

  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_drawing(*mHandle, kindMap[kind], layerMap[layer], &svg,
                           width->toMm(), filled);
}

void InteractiveHtmlBom::addTrack(
    Layer layer, const Point& start, const Point& end,
    const PositiveLength& width,
    const std::optional<QString>& netName) noexcept {
  static const QHash<Layer, rs::InteractiveHtmlBomLayer> map = {
      {Layer::Top, rs::InteractiveHtmlBomLayer::Front},
      {Layer::Bottom, rs::InteractiveHtmlBomLayer::Back},
  };
  rs::ffi_ibom_add_track(*mHandle, map[layer], start.getX().toMm(),
                         -start.getY().toMm(), end.getX().toMm(),
                         -end.getY().toMm(), width->toMm(),
                         netName ? &(*netName) : nullptr);
}

void InteractiveHtmlBom::addVia(
    QSet<Layer> layers, const Point& pos, const PositiveLength& diameter,
    const PositiveLength& drillDiameter,
    const std::optional<QString>& netName) noexcept {
  static const QHash<Layer, rs::InteractiveHtmlBomLayer> map = {
      {Layer::Top, rs::InteractiveHtmlBomLayer::Front},
      {Layer::Bottom, rs::InteractiveHtmlBomLayer::Back},
  };

  std::vector<rs::InteractiveHtmlBomLayer> layersVec;
  for (auto layer : layers) {
    layersVec.push_back(map[layer]);
  }
  const float drill = drillDiameter->toMm();
  rs::ffi_ibom_add_via(*mHandle, layersVec.data(), layersVec.size(),
                       pos.getX().toMm(), -pos.getY().toMm(), diameter->toMm(),
                       drill, netName ? &(*netName) : nullptr);
}

void InteractiveHtmlBom::addPlaneFragment(
    Layer layer, const Path& outline,
    const std::optional<QString>& netName) noexcept {
  static const QHash<Layer, rs::InteractiveHtmlBomLayer> map = {
      {Layer::Top, rs::InteractiveHtmlBomLayer::Front},
      {Layer::Bottom, rs::InteractiveHtmlBomLayer::Back},
  };
  const QString svg = outline.toClosedPath().toSvgPathMm();
  rs::ffi_ibom_add_zone(*mHandle, map[layer], &svg,
                        netName ? &(*netName) : nullptr);
}

std::size_t InteractiveHtmlBom::addFootprint(
    Layer layer, const Point& pos, const Angle& rot, const Point& topLeft,
    const Point& bottomRight, bool mount, const QStringList& fields,
    const QList<Pad>& pads) noexcept {
  static const QHash<Layer, rs::InteractiveHtmlBomLayer> map = {
      {Layer::Top, rs::InteractiveHtmlBomLayer::Front},
      {Layer::Bottom, rs::InteractiveHtmlBomLayer::Back},
  };

  std::vector<QString> svgs;
  svgs.reserve(pads.size());
  std::vector<rs::InteractiveHtmlBomPad> padsVec;
  // Important: Always allocate a non-empty pads vector even if there are no
  // pads. This makes sure the pointer passed to Rust is pointing to a valid
  // memory location. An empty vector caused a panic in debug mode which was
  // fixed by the line below.
  padsVec.reserve(pads.isEmpty() ? 1 : pads.size());
  for (const Pad& pad : pads) {
    // Determine hole.
    bool hasDrill = false;
    Length holeWidth, holeHeight;
    Point holeOffset;
    Angle holeRotation;
    if (auto h = pad.holes.value(0)) {
      if (h->isSlot()) {
        const Point p0 = h->getPath()->getVertices().at(0).getPos();
        const Point p1 = h->getPath()->getVertices().at(1).getPos();
        holeRotation =
            Toolbox::angleBetweenPoints(p0, p1) * (pad.mirrorGeometry ? -1 : 1);
        holeOffset = (p0 + p1) / 2;
        holeWidth = *(p1 - p0).getLength() + h->getDiameter();
        holeHeight = *h->getDiameter();
        hasDrill = true;
      } else {
        holeOffset = h->getPath()->getVertices().first().getPos();
        holeWidth = *h->getDiameter();
        holeHeight = *h->getDiameter();
        hasDrill = true;
      }
    }

    // Determine outline.
    const QVector<Path> outlines = pad.geometries.isEmpty()
        ? QVector<Path>()
        : pad.geometries.first().toOutlines();
    Path outline =
        outlines.isEmpty() ? Path() : outlines.first().toClosedPath();
    outline.translate(-holeOffset);
    if (pad.mirrorGeometry) {
      outline.mirror(Qt::Horizontal);
    }
    outline.rotate(-holeRotation);

    // Determine target transform.
    const Transform transform(pad.position, pad.rotation, pad.mirrorGeometry);
    const Point pos = transform.map(holeOffset);
    const Angle rot = pad.rotation + holeRotation;

    // Prepare data for FFI.
    svgs.emplace_back(outline.toSvgPathMm());
    padsVec.push_back(rs::InteractiveHtmlBomPad{
        pad.onTop,
        pad.onBottom,
        static_cast<float>(pos.getX().toMm()),
        static_cast<float>(-pos.getY().toMm()),
        static_cast<float>(rot.toDeg()),
        &svgs.back(),
        hasDrill,
        static_cast<float>(holeWidth.toMm()),
        static_cast<float>(holeHeight.toMm()),
        pad.netName ? &(*pad.netName) : nullptr,
        pad.pin1,
    });
  }
  Point origin = pos;
  if (layer == Layer::Bottom) {
    origin -= Point(bottomRight.getX() + topLeft.getX(), 0).rotated(rot);
  }
  return rs::ffi_ibom_add_footprint(
      *mHandle, map[layer], origin.getX().toMm(), -origin.getY().toMm(),
      rot.toDeg(), topLeft.getX().toMm(), -bottomRight.getY().toMm(),
      bottomRight.getX().toMm(), -topLeft.getY().toMm(), mount, &fields,
      padsVec.data(), padsVec.size());
}

void InteractiveHtmlBom::addBomRow(
    Sides sides,
    const QList<std::pair<QString, std::size_t> >& parts) noexcept {
  static const QHash<Sides, rs::InteractiveHtmlBomSides> map = {
      {Sides::Top, rs::InteractiveHtmlBomSides::Front},
      {Sides::Bottom, rs::InteractiveHtmlBomSides::Back},
      {Sides::Both, rs::InteractiveHtmlBomSides::Both},
  };

  std::vector<rs::InteractiveHtmlBomRefMap> list;
  for (const auto& part : parts) {
    list.push_back(rs::InteractiveHtmlBomRefMap{&part.first, part.second});
  }
  rs::ffi_ibom_add_bom_line(*mHandle, map[sides], list.data(), list.size());
}

QString InteractiveHtmlBom::generateHtml() const {
  QString out, err;
  if (!rs::ffi_ibom_generate_html(*mHandle, &out, &err)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to generate interactive HTML BOM: " % err);
  }
  return out;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
