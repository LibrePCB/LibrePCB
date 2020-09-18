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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDNETSEGMENTSPLITTER_H
#define LIBREPCB_PROJECT_EDITOR_BOARDNETSEGMENTSPLITTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/geometry/junction.h>
#include <librepcb/common/geometry/trace.h>
#include <librepcb/common/geometry/via.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {
namespace editor {

/*******************************************************************************
 *  Class BoardNetSegmentSplitter
 ******************************************************************************/

/**
 * @brief The BoardNetSegmentSplitter class
 */
class BoardNetSegmentSplitter final {
public:
  // Types
  struct Segment {
    JunctionList junctions;
    ViaList vias;
    TraceList traces;
  };

  // Constructors / Destructor
  BoardNetSegmentSplitter() noexcept;
  BoardNetSegmentSplitter(const BoardNetSegmentSplitter& other) = delete;
  ~BoardNetSegmentSplitter() noexcept;

  // General Methods
  void replaceFootprintPadByJunctions(const TraceAnchor& anchor,
                                      const Point& pos) noexcept;
  void addJunction(const Junction& junction) noexcept;
  void addVia(const Via& via, bool replaceByJunctions) noexcept;
  void addTrace(const Trace& trace) noexcept;
  QList<Segment> split() noexcept;

  // Operator Overloadings
  BoardNetSegmentSplitter& operator=(const BoardNetSegmentSplitter& rhs) =
      delete;

private:  // Methods
  TraceAnchor replaceAnchor(const TraceAnchor& anchor,
                            const GraphicsLayerName& layer) noexcept;
  void findConnectedLinesAndPoints(const TraceAnchor& anchor,
                                   ViaList& availableVias,
                                   TraceList& availableTraces, Segment& segment)

      noexcept;

private:  // Data
  JunctionList mJunctions;
  ViaList mVias;
  TraceList mTraces;

  QHash<TraceAnchor, Point> mAnchorsToReplace;
  QHash<QPair<TraceAnchor, GraphicsLayerName>, TraceAnchor> mReplacedAnchors;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
