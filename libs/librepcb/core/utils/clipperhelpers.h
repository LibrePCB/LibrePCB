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

#include <polyclipping/clipper.hpp>

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
  static void unite(ClipperLib::Paths& paths,
                    ClipperLib::PolyFillType fillType);
  static void unite(ClipperLib::Paths& subject, const ClipperLib::Paths& clip,
                    ClipperLib::PolyFillType subjectFillType,
                    ClipperLib::PolyFillType clipFillType);
  static std::unique_ptr<ClipperLib::PolyTree> uniteToTree(
      const ClipperLib::Paths& paths, ClipperLib::PolyFillType fillType);
  static std::unique_ptr<ClipperLib::PolyTree> uniteToTree(
      const ClipperLib::Paths& paths, const ClipperLib::Paths& clip,
      ClipperLib::PolyFillType subjectFillType,
      ClipperLib::PolyFillType clipFillType);
  static void intersect(ClipperLib::Paths& subject,
                        const ClipperLib::Paths& clip,
                        ClipperLib::PolyFillType subjectFillType,
                        ClipperLib::PolyFillType clipFillType);
  static std::unique_ptr<ClipperLib::PolyTree> intersectToTree(
      const ClipperLib::Paths& subject, const ClipperLib::Paths& clip,
      ClipperLib::PolyFillType subjectFillType,
      ClipperLib::PolyFillType clipFillType);
  static std::unique_ptr<ClipperLib::PolyTree> intersectToTree(
      const QList<ClipperLib::Paths>& paths);
  static void subtract(ClipperLib::Paths& subject,
                       const ClipperLib::Paths& clip,
                       ClipperLib::PolyFillType subjectFillType,
                       ClipperLib::PolyFillType clipFillType);
  static std::unique_ptr<ClipperLib::PolyTree> subtractToTree(
      const ClipperLib::Paths& subject, const ClipperLib::Paths& clip,
      ClipperLib::PolyFillType subjectFillType,
      ClipperLib::PolyFillType clipFillType);
  static void offset(ClipperLib::Paths& paths, const Length& offset,
                     const PositiveLength& maxArcTolerance);
  static std::unique_ptr<ClipperLib::PolyTree> offsetToTree(
      const ClipperLib::Paths& paths, const Length& offset,
      const PositiveLength& maxArcTolerance);
  static ClipperLib::Paths treeToPaths(const ClipperLib::PolyTree& tree);
  static ClipperLib::Paths flattenTree(const ClipperLib::PolyNode& node);

  // Type Conversions
  static QVector<Path> convert(const ClipperLib::Paths& paths) noexcept;
  static Path convert(const ClipperLib::Path& path) noexcept;
  static Point convert(const ClipperLib::IntPoint& point) noexcept;
  static ClipperLib::Paths convert(
      const QVector<Path>& paths,
      const PositiveLength& maxArcTolerance) noexcept;
  static ClipperLib::Path convert(
      const Path& path, const PositiveLength& maxArcTolerance) noexcept;
  static ClipperLib::IntPoint convert(const Point& point) noexcept;

private:  // Internal Helper Methods
  static ClipperLib::Path convertHolesToCutIns(const ClipperLib::Path& outline,
                                               const ClipperLib::Paths& holes);
  static ClipperLib::Paths prepareHoles(
      const ClipperLib::Paths& holes) noexcept;
  static ClipperLib::Path rotateCutInHole(
      const ClipperLib::Path& hole) noexcept;
  static int getHoleConnectionPointIndex(const ClipperLib::Path& hole) noexcept;
  static void addCutInToPath(ClipperLib::Path& outline,
                             const ClipperLib::Path& hole);
  static int insertConnectionPointToPath(ClipperLib::Path& path,
                                         const ClipperLib::IntPoint& p);
  static bool calcIntersectionPos(const ClipperLib::IntPoint& p1,
                                  const ClipperLib::IntPoint& p2,
                                  const ClipperLib::cInt& x,
                                  ClipperLib::cInt& y) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
