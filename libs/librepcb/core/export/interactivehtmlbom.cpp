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

#include <librepcb/core/geometry/path.h>
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

void InteractiveHtmlBom::addEdge(const Path& path) noexcept {
  const QString svg = path.toSvgPathMm();
  rs::ffi_ibom_add_edge(*mHandle, &svg, 0.1f, false);
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

std::size_t InteractiveHtmlBom::addFootprint(const QString& name, bool mirror,
                                             const Point& pos, const Angle& rot,
                                             const Length& minX,
                                             const Length& maxX,
                                             const Length& minY,
                                             const Length& maxY) noexcept {
  return rs::ffi_ibom_add_footprint(*mHandle, &name,
                                    mirror ? rs::InteractiveHtmlBomLayer::Back
                                           : rs::InteractiveHtmlBomLayer::Front,
                                    pos.getX().toMm(), -pos.getY().toMm(),
                                    rot.toDeg(), minX.toMm(), -maxY.toMm(),
                                    (maxX - minX).toMm(), (maxY - minY).toMm());
}

void InteractiveHtmlBom::addBomRow(
    const QList<std::pair<QString, std::size_t> >& parts) noexcept {
  std::vector<rs::InteractiveHtmlBomRefMap> list;
  for (const auto& part : parts) {
    list.emplace_back(&part.first, part.second);
  }
  rs::ffi_ibom_add_bom_line(*mHandle, rs::InteractiveHtmlBomLayer::Front,
                            list.data(), list.size());
}

QString InteractiveHtmlBom::generate() const noexcept {
  QString out;
  rs::ffi_ibom_generate(*mHandle, &out);
  return out;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
