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

#ifndef LIBREPCB_CORE_NETSEGMENTSIMPLIFIER_H
#define LIBREPCB_CORE_NETSEGMENTSIMPLIFIER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class NetSegmentSimplifier
 ******************************************************************************/

/**
 * @brief Algorithm to clean/simplify net segment lines
 *
 * Performed operations:
 *  - Remove redundant junctions (same position, same layer)
 *  - Remove redundant lines (same anchors, same layer), keeping the thickest
 *  - Remove zero-length lines
 *  - Remove useless junctions within straight line segments (join line
 *    segments into the same direction to a single line)
 *  - Split lines to connect with junctions on the path between start and end
 *    points
 *  - Split intersecting lines, placing a new junction to connect them (only
 *    orthogonal intersections for now)
 */
class NetSegmentSimplifier final {
  Q_DECLARE_TR_FUNCTIONS(NetSegmentSimplifier)

public:
  // Types
  enum class AnchorType : int {
    // Value is important for the sort algorithm, do not change!
    Via = 0,
    PinOrPad = 1,
    Junction = 2,
  };
  struct Line {
    int id = 0;
    int p1 = 0;
    int p2 = 0;
    const Layer* layer = nullptr;
    Length width;
    bool modified = false;
  };
  struct Result {
    QList<Line> lines;
    QMap<int, Point> newJunctions;
    QSet<int> disconnectedPinsOrPads;
    bool modified = false;
  };

  // Constructors / Destructor

  /**
   * @brief Default constructor
   */
  NetSegmentSimplifier() noexcept;

  /**
   * @brief Copy constructor
   *
   * @param other     Another ::librepcb::NetSegmentSimplifier object
   */
  NetSegmentSimplifier(const NetSegmentSimplifier& other) = delete;

  /**
   * Destructor
   */
  ~NetSegmentSimplifier() noexcept;

  /**
   * @brief Add a line anchor
   *
   * @param type     Type of the anchor.
   * @param pos      Position.
   * @param start    Start (most upper) layer of the anchors or `nullptr`
   *                 for schematic netsegment simplifications
   * @param end      End (most lower) layer of the anchors or `nullptr`
   *                 for schematic netsegment simplifications
   *
   * @return The ID of the added anchors.
   */
  int addAnchor(AnchorType type, const Point& pos, const Layer* start,
                const Layer* end) noexcept;

  /**
   * @brief Add a line between two anchors
   *
   * @param p1      ID of first anchors.
   * @param p2      ID of second anchors.
   * @param layer   Layer of the line (`nullptr` for schematic netsegment
   *                simplification).
   * @param width   Line width.
   *
   * @return The ID of the added line.
   */
  int addLine(int p1, int p2, const Layer* layer, const Length& width) noexcept;

  /**
   * @brief Perform the simplification
   *
   * @note  This method also resets the state, so the object can be reused for
   *        the next net segment.
   *
   * @attention When lines are split, new anchor- and line IDs will be
   *            generated on the fly! So the returned lines may contain IDs
   *            which you didn't know yet from #addAnchor() and #addLine()!.
   *
   * @return   Any remaining lines after the simplification.
   */
  Result simplify() noexcept;

  // Operator overloadings
  NetSegmentSimplifier& operator=(const NetSegmentSimplifier& rhs) = delete;

private:
  // Types
  struct Anchor {
    int id = 0;
    AnchorType type = AnchorType::Junction;
    Point pos;
    const Layer* startLayer = nullptr;
    const Layer* endLayer = nullptr;
    bool isNew = false;
  };

  // Methods
  QSet<int> getConnectedPinsOrPads() const noexcept;
  void addJunctionsAtLineIntersections() noexcept;
  void splitLinesAtAnchors() noexcept;
  void removeDuplicateJunctions() noexcept;
  void removeRedundantLines() noexcept;
  bool mergeNextLines() noexcept;
  const Anchor* findAnchor(const Point& pos, const Layer* layer) noexcept;
  static bool isAnchorOnLayer(const Anchor& anchor,
                              const Layer* layer) noexcept;
  static bool isStraightLine(const Point& p0, const Point& p1,
                             const Point& p2) noexcept;

  // Data input
  QList<Anchor> mAnchors;
  QMap<int, Line> mLines;
  int mNextFreeLineId;

  // State
  QHash<Point, QVector<Anchor>> mAnchorMap;
  QSet<int> mPinsOrPads;
  bool mModified;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
