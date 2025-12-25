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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_DRAWBUS_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_DRAWBUS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"
#include "schematiceditorstate_drawwire.h"

#include <librepcb/core/types/point.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class SI_BusJunction;
class SI_BusLabel;
class SI_BusLine;
class SI_BusSegment;

namespace editor {

/*******************************************************************************
 *  Class SchematicEditorState_DrawBus
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_DrawBus class
 */
class SchematicEditorState_DrawBus final : public SchematicEditorState {
  Q_OBJECT

  /// Internal FSM States (substates)
  enum class SubState {
    IDLE,  ///< idle state [initial state]
    POSITIONING_JUNCTION  ///< in this state, an undo command is active!
  };

public:
  using WireMode = SchematicEditorState_DrawWire::WireMode;

  // Constructors / Destructor
  SchematicEditorState_DrawBus() = delete;
  SchematicEditorState_DrawBus(const SchematicEditorState_DrawBus& other) =
      delete;
  explicit SchematicEditorState_DrawBus(const Context& context) noexcept;
  virtual ~SchematicEditorState_DrawBus() noexcept;

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
  void selectBus(const std::optional<Uuid> uuid) noexcept;
  WireMode getWireMode() const noexcept { return mCurrentWireMode; }
  void setWireMode(WireMode mode) noexcept;

  // Operator Overloadings
  SchematicEditorState_DrawBus& operator=(
      const SchematicEditorState_DrawBus& rhs) = delete;

signals:
  void wireModeChanged(WireMode mode);

private:  //  Methods
  bool startPositioning(SchematicGraphicsScene& scene, bool snap,
                        SI_BusJunction* fixedPoint = nullptr) noexcept;
  bool addNextJunction(SchematicGraphicsScene& scene, bool snap) noexcept;
  bool abortPositioning(bool showErrMsgBox, bool simplifySegment) noexcept;
  std::shared_ptr<QGraphicsItem> findItem(
      const Point& pos,
      const QVector<std::shared_ptr<QGraphicsItem>>& except = {}) noexcept;
  Point updateJunctionPositions(bool snap) noexcept;
  void updateLabelPosition(const Point& pos, const Point& dirPos) noexcept;
  Point calcMiddlePointPos(const Point& p1, const Point p2,
                           WireMode mode) const noexcept;

private:  // Data
  Circuit& mCircuit;
  SubState mSubState;  ///< the current substate
  WireMode mCurrentWireMode;  ///< the current wire mode
  Point mCursorPos;  ///< the current cursor position
  SI_BusJunction* mFixedStartAnchor;  ///< the fixed anchor (start point of
                                      ///< the line)
  SI_BusSegment* mCurrentSegment;  ///< the net segment that is currently
                                   ///< edited
  SI_BusLine* mPositioningLine1;  ///< line between fixed point and p1
  SI_BusJunction* mPositioningJunction1;  ///< the first netpoint to place
  SI_BusLine* mPositioningLine2;  ///< line between p1 and p2
  SI_BusJunction* mPositioningJunction2;  ///< the second netpoint to place
  SI_BusLabel* mPositioningLabel;  ///< Only if bus was preselected

  std::optional<Uuid> mPreSelectedBus;  ///< If tool was started with given bus
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
