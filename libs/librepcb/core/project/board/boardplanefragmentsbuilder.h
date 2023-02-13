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

#ifndef LIBREPCB_CORE_BOARDPLANEFRAGMENTSBUILDER_H
#define LIBREPCB_CORE_BOARDPLANEFRAGMENTSBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/path.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_FootprintPad;
class BI_Plane;
class BI_Via;
class Transform;

/*******************************************************************************
 *  Class BoardPlaneFragmentsBuilder
 ******************************************************************************/

/**
 * @brief The BoardPlaneFragmentsBuilder class
 */
class BoardPlaneFragmentsBuilder final {
public:
  // Constructors / Destructor
  BoardPlaneFragmentsBuilder() = delete;
  BoardPlaneFragmentsBuilder(const BoardPlaneFragmentsBuilder& other) = delete;
  BoardPlaneFragmentsBuilder(BI_Plane& plane) noexcept;
  ~BoardPlaneFragmentsBuilder() noexcept;

  // General Methods
  QVector<Path> buildFragments() noexcept;

  // Operator Overloadings
  BoardPlaneFragmentsBuilder& operator=(const BoardPlaneFragmentsBuilder& rhs) =
      delete;

private:  // Methods
  void addPlaneOutline();
  void clipToBoardOutline();
  void subtractOtherObjects();
  void ensureMinimumWidth();
  void flattenResult();
  void removeOrphans();

  // Helper Methods
  ClipperLib::Paths createPadCutOuts(const Transform& deviceTransform,
                                     const Transform& padTransform,
                                     const BI_FootprintPad& pad) const;
  ClipperLib::Path createViaCutOut(const BI_Via& via) const noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs. Do not
   * change this if you don't know exactly what you're doing (it affects all
   * planes in all existing boards)!
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  BI_Plane& mPlane;
  ClipperLib::Paths mConnectedNetSignalAreas;
  ClipperLib::Paths mResult;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
