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

#ifndef LIBREPCB_CORE_CLIPPERHELPERS_H
#define LIBREPCB_CORE_CLIPPERHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"

#include <clipper2/clipper.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ClipperHelpers
 ******************************************************************************/

/**
 * @brief The ClipperHelpers class
 */
class ClipperHelpers final {
  Q_DECLARE_TR_FUNCTIONS(ClipperHelpers)

public:
  // Disable instantiation
  ClipperHelpers() = delete;
  ~ClipperHelpers() = delete;

  // General Methods
  static bool allPointsInside(const Clipper2Lib::Path64& points,
                              const Clipper2Lib::Path64& path);
  static bool anyPointsInside(const Clipper2Lib::Path64& points,
                              const Clipper2Lib::Path64& path);
  static bool anyPointsInside(const Clipper2Lib::Paths64& points,
                              const Clipper2Lib::Path64& path);
  static void unite(Clipper2Lib::Paths64& paths,
                    Clipper2Lib::FillRule fillType);
  static void unite(Clipper2Lib::Paths64& subject,
                    const Clipper2Lib::Paths64& clip,
                    Clipper2Lib::FillRule fillType);
  static std::unique_ptr<Clipper2Lib::PolyTree64> uniteToTree(
      const Clipper2Lib::Paths64& paths, Clipper2Lib::FillRule fillType);
  static std::unique_ptr<Clipper2Lib::PolyTree64> uniteToTree(
      const Clipper2Lib::Paths64& paths, const Clipper2Lib::Paths64& clip,
      Clipper2Lib::FillRule fillType);
  static void intersect(Clipper2Lib::Paths64& subject,
                        const Clipper2Lib::Paths64& clip,
                        Clipper2Lib::FillRule fillType);
  static std::unique_ptr<Clipper2Lib::PolyTree64> intersectToTree(
      const Clipper2Lib::Paths64& subject, const Clipper2Lib::Paths64& clip,
      Clipper2Lib::FillRule fillType,
      bool closed = true);
  static std::unique_ptr<Clipper2Lib::PolyTree64> intersectToTree(
      const QList<Clipper2Lib::Paths64>& paths);
  static void subtract(Clipper2Lib::Paths64& subject,
                       const Clipper2Lib::Paths64& clip,
                       Clipper2Lib::FillRule fillType);
  static std::unique_ptr<Clipper2Lib::PolyTree64> subtractToTree(
      const Clipper2Lib::Paths64& subject, const Clipper2Lib::Paths64& clip,
      Clipper2Lib::FillRule fillType,
      bool closed = true);
  static void offset(
      Clipper2Lib::Paths64& paths, const Length& offset,
      const PositiveLength& maxArcTolerance,
      Clipper2Lib::JoinType joinType = Clipper2Lib::JoinType::Round);
  static std::unique_ptr<Clipper2Lib::PolyTree64> offsetToTree(
      const Clipper2Lib::Paths64& paths, const Length& offset,
      const PositiveLength& maxArcTolerance);
  static Clipper2Lib::Paths64 treeToPaths(const Clipper2Lib::PolyTree64& tree);
  static Clipper2Lib::Paths64 flattenTree(const Clipper2Lib::PolyTree64& node);

  // Type Conversions
  static QVector<Path> convert(const Clipper2Lib::Paths64& paths) noexcept;
  static Path convert(const Clipper2Lib::Path64& path) noexcept;
  static Point convert(const Clipper2Lib::Point64& point) noexcept;
  static Clipper2Lib::Paths64 convert(
      const QVector<Path>& paths,
      const PositiveLength& maxArcTolerance) noexcept;
  static Clipper2Lib::Path64 convert(
      const Path& path, const PositiveLength& maxArcTolerance) noexcept;
  static Clipper2Lib::Point64 convert(const Point& point) noexcept;

private:  // Internal Helper Methods
  static Clipper2Lib::Path64 convertHolesToCutIns(
      const Clipper2Lib::Path64& outline, const Clipper2Lib::Paths64& holes);
  static Clipper2Lib::Paths64 prepareHoles(
      const Clipper2Lib::Paths64& holes) noexcept;
  static Clipper2Lib::Path64 rotateCutInHole(
      const Clipper2Lib::Path64& hole) noexcept;
  static void addCutInToPath(Clipper2Lib::Path64& outline,
                             const Clipper2Lib::Path64& hole);
  static int insertConnectionPointToPath(Clipper2Lib::Path64& path,
                                         const Clipper2Lib::Point64& p);
  static bool calcIntersectionPos(const Clipper2Lib::Point64& p1,
                                  const Clipper2Lib::Point64& p2,
                                  const int64_t& x, int64_t& y) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
