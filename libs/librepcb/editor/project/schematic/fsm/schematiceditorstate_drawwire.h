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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_DRAWWIRE_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_DRAWWIRE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class SI_NetLine;
class SI_NetLineAnchor;
class SI_NetPoint;
class SI_SymbolPin;

namespace editor {

/*******************************************************************************
 *  Class SchematicEditorState_DrawWire
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_DrawWire class
 */
class SchematicEditorState_DrawWire final : public SchematicEditorState {
  Q_OBJECT

  /// Internal FSM States (substates)
  enum class SubState {
    IDLE,  ///< idle state [initial state]
    POSITIONING_NETPOINT  ///< in this state, an undo command is active!
  };

public:
  /**
   * @brief All available wire modes
   */
  enum class WireMode {
    HV,  ///< horizontal - vertical [default]
    VH,  ///< vertical - horizontal
    Deg9045,  ///< 90° - 45°
    Deg4590,  ///< 45° - 90°
    Straight,  ///< straight
    _COUNT,
  };

  // Constructors / Destructor
  SchematicEditorState_DrawWire() = delete;
  SchematicEditorState_DrawWire(const SchematicEditorState_DrawWire& other) =
      delete;
  explicit SchematicEditorState_DrawWire(const Context& context) noexcept;
  virtual ~SchematicEditorState_DrawWire() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept override;
  virtual bool processKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Connection to UI
  WireMode getWireMode() const noexcept { return mCurrentWireMode; }
  void setWireMode(WireMode mode) noexcept;

  // Operator Overloadings
  SchematicEditorState_DrawWire& operator=(
      const SchematicEditorState_DrawWire& rhs) = delete;

signals:
  void wireModeChanged(WireMode mode);

private:  //  Methods
  bool startPositioning(SchematicGraphicsScene& scene, bool snap,
                        SI_NetPoint* fixedPoint = nullptr) noexcept;
  bool addNextNetPoint(SchematicGraphicsScene& scene, bool snap) noexcept;
  bool abortPositioning(bool showErrMsgBox) noexcept;
  std::shared_ptr<QGraphicsItem> findItem(
      const Point& pos,
      const QVector<std::shared_ptr<QGraphicsItem>>& except = {}) noexcept;
  Point updateNetpointPositions(bool snap) noexcept;
  Point calcMiddlePointPos(const Point& p1, const Point p2,
                           WireMode mode) const noexcept;

private:  // Data
  Circuit& mCircuit;
  SubState mSubState;  ///< the current substate
  WireMode mCurrentWireMode;  ///< the current wire mode
  Point mCursorPos;  ///< the current cursor position
  SI_NetLineAnchor*
      mFixedStartAnchor;  ///< the fixed anchor (start point of the line)
  SI_NetLine* mPositioningNetLine1;  ///< line between fixed point and p1
  SI_NetPoint* mPositioningNetPoint1;  ///< the first netpoint to place
  SI_NetLine* mPositioningNetLine2;  ///< line between p1 and p2
  SI_NetPoint* mPositioningNetPoint2;  ///< the second netpoint to place
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
