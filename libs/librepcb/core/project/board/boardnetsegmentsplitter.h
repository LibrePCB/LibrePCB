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

#ifndef LIBREPCB_CORE_BOARDNETSEGMENTSPLITTER_H
#define LIBREPCB_CORE_BOARDNETSEGMENTSPLITTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/junction.h"
#include "../../geometry/trace.h"
#include "../../geometry/via.h"
#include "boardpaddata.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

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
    BoardPadDataList pads;
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
  void addPad(const BoardPadData& pad, bool replaceByJunctions) noexcept;
  void addVia(const Via& via, bool replaceByJunctions) noexcept;
  void addTrace(const Trace& trace) noexcept;
  QList<Segment> split() noexcept;

  // Operator Overloadings
  BoardNetSegmentSplitter& operator=(const BoardNetSegmentSplitter& rhs) =
      delete;

private:  // Methods
  TraceAnchor replaceAnchor(const TraceAnchor& anchor,
                            const Layer& layer) noexcept;
  void findConnectedLinesAndPoints(
      const TraceAnchor& anchor,
      QList<std::shared_ptr<BoardPadData>>& availablePads,
      QList<std::shared_ptr<Via>>& availableVias,
      QList<std::shared_ptr<Trace>>& availableTraces, Segment& segment)

      noexcept;

private:  // Data
  JunctionList mJunctions;
  BoardPadDataList mPads;
  ViaList mVias;
  TraceList mTraces;

  QHash<TraceAnchor, Point> mAnchorsToReplace;
  QHash<QPair<TraceAnchor, const Layer*>, TraceAnchor> mReplacedAnchors;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
