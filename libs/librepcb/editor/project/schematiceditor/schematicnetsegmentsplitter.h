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

#ifndef LIBREPCB_PROJECTEDITOR_SCHEMATICNETSEGMENTSPLITTER_H
#define LIBREPCB_PROJECTEDITOR_SCHEMATICNETSEGMENTSPLITTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/geometry/junction.h>
#include <librepcb/common/geometry/netlabel.h>
#include <librepcb/common/geometry/netline.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Class SchematicNetSegmentSplitter
 ******************************************************************************/

/**
 * @brief The SchematicNetSegmentSplitter class
 */
class SchematicNetSegmentSplitter final {
public:
  // Types
  struct Segment {
    JunctionList junctions;
    NetLineList netlines;
    NetLabelList netlabels;
  };

  // Constructors / Destructor
  SchematicNetSegmentSplitter() noexcept;
  SchematicNetSegmentSplitter(const SchematicNetSegmentSplitter& other) =
      delete;
  ~SchematicNetSegmentSplitter() noexcept;

  // General Methods
  void addSymbolPin(const NetLineAnchor& anchor, const Point& pos,
                    bool replaceByJunction = false) noexcept;
  void addJunction(const Junction& junction) noexcept;
  void addNetLine(const NetLine& netline) noexcept;
  void addNetLabel(const NetLabel& netlabel) noexcept;
  QList<Segment> split() noexcept;

  // Operator Overloadings
  SchematicNetSegmentSplitter& operator=(
      const SchematicNetSegmentSplitter& rhs) = delete;

private:  // Methods
  NetLineAnchor replacePinAnchor(const NetLineAnchor& anchor) noexcept;
  void findConnectedLinesAndPoints(const NetLineAnchor& anchor,
                                   NetLineList& availableNetLines,
                                   Segment& segment)

      noexcept;
  void addNetLabelToNearestNetSegment(const NetLabel& netlabel,
                                      QList<Segment>& segments) const noexcept;
  Length getDistanceBetweenNetLabelAndNetSegment(
      const NetLabel& netlabel, const Segment& netsegment) const noexcept;
  Point getAnchorPosition(const NetLineAnchor& anchor) const noexcept;

private:  // Data
  JunctionList mJunctions;
  NetLineList mNetLines;
  NetLabelList mNetLabels;

  QHash<NetLineAnchor, NetLineAnchor> mPinAnchorsToReplace;
  QHash<NetLineAnchor, Point> mPinPositions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
