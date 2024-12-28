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

/*******************************************************************************
 *  Class InteractiveHtmlBom
 ******************************************************************************/

static RustHandle<rs::InteractiveHtmlBom> construct(
    const QString& title, const QString& revision, const QString& company,
    const QString& date, const Length& minX, const Length& maxX,
    const Length& minY, const Length& maxY) {
  if (auto obj =
          rs::ffi_ibom_new(&title, &revision, &company, &date, minX.toMm(),
                           maxX.toMm(), -maxY.toMm(), -minY.toMm())) {
    return RustHandle<rs::InteractiveHtmlBom>(*obj, &rs::ffi_ibom_delete);
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Failed to create InteractiveHtmlBom"));
  }
}

InteractiveHtmlBom::InteractiveHtmlBom(const QString& title,
                                       const QString& revision,
                                       const QString& company,
                                       const QString& date, const Length& minX,
                                       const Length& maxX, const Length& minY,
                                       const Length& maxY)
  : mHandle(construct(title, revision, company, date, minX, maxX, minY, maxY)) {
}

void InteractiveHtmlBom::setExtraFields(const QStringList& fields) noexcept {
  rs::ffi_ibom_set_fields(*mHandle, &fields);
}

void InteractiveHtmlBom::addEdge(const Path& path) noexcept {
  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_edge(*mHandle, &svg, 0.0f, false);
}

void InteractiveHtmlBom::addLegendTop(const Path& path,
                                      const UnsignedLength& width,
                                      bool fill) noexcept {
  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_silkscreen_front(*mHandle, &svg, width->toMm(), fill);
}

void InteractiveHtmlBom::addLegendBot(const Path& path,
                                      const UnsignedLength& width,
                                      bool fill) noexcept {
  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_silkscreen_back(*mHandle, &svg, width->toMm(), fill);
}

void InteractiveHtmlBom::addDocumentationTop(const Path& path,
                                             const UnsignedLength& width,
                                             bool fill) noexcept {
  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_fabrication_front(*mHandle, &svg, width->toMm(), fill);
}

void InteractiveHtmlBom::addDocumentationBot(const Path& path,
                                             const UnsignedLength& width,
                                             bool fill) noexcept {
  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_fabrication_back(*mHandle, &svg, width->toMm(), fill);
}

std::size_t InteractiveHtmlBom::addFootprint(
    bool mirror, const Point& pos, const Angle& rot, const Length& minX,
    const Length& maxX, const Length& minY, const Length& maxY, bool mount,
    const QStringList& fields, const QList<Pad>& pads) noexcept {
  std::vector<QString> svgs;
  svgs.reserve(pads.size());
  std::vector<rs::InteractiveHtmlBomPad> padsVec;
  padsVec.reserve(pads.size());
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
    });
  }
  return rs::ffi_ibom_add_footprint(
      *mHandle,
      mirror ? rs::InteractiveHtmlBomLayer::Back
             : rs::InteractiveHtmlBomLayer::Front,
      pos.getX().toMm(), -pos.getY().toMm(), rot.toDeg(), minX.toMm(),
      -maxY.toMm(), (maxX - minX).toMm(), (maxY - minY).toMm(), mount, &fields,
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
    list.emplace_back(&part.first, part.second);
  }
  rs::ffi_ibom_add_bom_line(*mHandle, map[sides], list.data(), list.size());
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
                         -end.getY().toMm(), width->toMm(), nullptr,
                         netName ? &(*netName) : nullptr);
}

void InteractiveHtmlBom::addVia(
    Layer layer, const Point& pos, const PositiveLength& diameter,
    const PositiveLength& drillDiameter,
    const std::optional<QString>& netName) noexcept {
  static const QHash<Layer, rs::InteractiveHtmlBomLayer> map = {
      {Layer::Top, rs::InteractiveHtmlBomLayer::Front},
      {Layer::Bottom, rs::InteractiveHtmlBomLayer::Back},
  };
  const float drill = drillDiameter->toMm();
  rs::ffi_ibom_add_track(*mHandle, map[layer], pos.getX().toMm(),
                         -pos.getY().toMm(), pos.getX().toMm(),
                         -pos.getY().toMm(), diameter->toMm(), &drill,
                         netName ? &(*netName) : nullptr);
}

void InteractiveHtmlBom::addPlaneFragment(
    Layer layer, const Path& fragment,
    const std::optional<QString>& netName) noexcept {
  static const QHash<Layer, rs::InteractiveHtmlBomLayer> map = {
      {Layer::Top, rs::InteractiveHtmlBomLayer::Front},
      {Layer::Bottom, rs::InteractiveHtmlBomLayer::Back},
  };
  const QString svg = fragment.toClosedPath().toSvgPathMm();
  rs::ffi_ibom_add_zone(*mHandle, map[layer], &svg,
                        netName ? &(*netName) : nullptr);
}

QString InteractiveHtmlBom::generate() const {
  QString out, err;
  if (!rs::ffi_ibom_generate(*mHandle, &out, &err)) {
    throw RuntimeError(__FILE__, __LINE__, "Failed to generate IBOM: " % err);
  }
  return out;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
