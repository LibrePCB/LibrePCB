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

#ifndef LIBREPCB_CORE_BOARDCLIPPERPATHGENERATOR_H
#define LIBREPCB_CORE_BOARDCLIPPERPATHGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/path.h"
#include "../../../types/length.h"
#include "../../../utils/transform.h"
#include "boarddesignrulecheckdata.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BoardClipperPathGenerator
 ******************************************************************************/

/**
 * @brief Helper to create Clipper paths for ::librepcb::BoardDesignRuleCheck
 */
class BoardClipperPathGenerator final {
public:
  using Data = BoardDesignRuleCheckData;

  // Constructors / Destructor
  explicit BoardClipperPathGenerator(
      const PositiveLength& maxArcTolerance) noexcept;
  ~BoardClipperPathGenerator() noexcept;

  // Getters
  const ClipperLib::Paths& getPaths() const noexcept { return mPaths; }
  void takePathsTo(ClipperLib::Paths& out) noexcept;

  // General Methods
  void addCopper(const Data& data, const Layer& layer,
                 const QSet<tl::optional<Uuid>>& netsignals,
                 bool ignorePlanes = false);
  void addStopMaskOpenings(const Data& data, const Layer& layer,
                           const Length& offset = Length(0));
  void addVia(const Data::Via& via, const Length& offset = Length(0));
  void addTrace(const Data::Trace& trace, const Length& offset = Length(0));
  void addPlane(const QVector<Path>& fragments);
  void addPolygon(const Path& path, const UnsignedLength& lineWidth,
                  bool filled, const Length& offset = Length(0));
  void addCircle(const Data::Circle& circle, const Transform& transform,
                 const Length& offset = Length(0));
  void addStrokeText(const Data::StrokeText& strokeText,
                     const Length& offset = Length(0));
  void addHole(const PositiveLength& diameter, const NonEmptyPath& path,
               const Transform& transform = Transform(),
               const Length& offset = Length(0));
  void addPad(const Data::Pad& pad, const Layer& layer,
              const Length& offset = Length(0));

private:  // Data
  PositiveLength mMaxArcTolerance;
  ClipperLib::Paths mPaths;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
