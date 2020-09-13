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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_SIMPLIFY_H
#define LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_SIMPLIFY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class BI_NetSegment;
class BI_Base;
class NetSignal;

namespace editor {

/*******************************************************************************
 *  Class BoardEditorState_Simplify
 ******************************************************************************/

/**
 * @brief The "simplify" state/tool of the board editor
 */
class BoardEditorState_Simplify final : public BoardEditorState {
  Q_OBJECT
public:
  // Constructors / Destructor
  BoardEditorState_Simplify()                                       = delete;
  BoardEditorState_Simplify(const BoardEditorState_Simplify& other) = delete;
  explicit BoardEditorState_Simplify(const Context& context) noexcept;
  ~BoardEditorState_Simplify();

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  BoardEditorState_Simplify& operator=(const BoardEditorState_Simplify& rhs) =
      delete;

private:
  /**
   * @brief Simplify the netsignal at the specified position.
   *
   * Every segment of the found netsignal will be simplified.
   *
   * @param board The board of the netsignal.
   * @param pos The search position of the netsignal.
   */
  void simplify(Board& board, Point& pos) noexcept;

  /**
   * @brief Simplify a segment.
   *
   * Simplifies the specified segment by:
   *   - removing duplicate netlines (same start and end points)
   *   - removing duplicate netpoints (same position and layer)
   *   - connecting netpoints and vias with netlines at the same position (and
   * layer)
   *   - remove intermitting points of multiple netlines forming a straigt line
   *
   * @param segment The segment to be simplified.
   */
  void simplifySegment(BI_NetSegment& segment) noexcept;

  /**
   * @brief Remove duplicate netlines.
   *
   * Keep only one netline, per layer, when two netpoints are connected by
   * multiple netlines.
   *
   * @param segment The segment which will be cleaned.
   */
  void removeDuplicateNetLines(BI_NetSegment& segment) noexcept;

  /**
   * @brief Combine duplicate netpoints of the segment.
   * @param segment The segment which will be cleaned.
   */
  void             combineDuplicateNetPoints(BI_NetSegment& segment) noexcept;
  QSet<NetSignal*> findNetSignals(Board& board, Point& pos,
                                  QSet<BI_Base*> except = {}) const noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_SIMPLIFY_H
