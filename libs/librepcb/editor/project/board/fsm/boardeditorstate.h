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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicsscene.h"
#include "boardeditorfsm.h"
#include "boardeditorfsmadapter.h"

#include <librepcb/core/types/length.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Layer;
class LengthUnit;
class NetSignal;
class Point;

namespace editor {

class BoardGraphicsScene;
class UndoCommand;

/*******************************************************************************
 *  Class BoardEditorState
 ******************************************************************************/

/**
 * @brief The board editor state base class
 */
class BoardEditorState : public QObject {
  Q_OBJECT

public:
  using Context = BoardEditorFsm::Context;

  enum class FindFlag : uint32_t {
    // Item types
    Vias = (1 << 0),
    NetPoints = (1 << 1),
    NetLines = (1 << 2),
    Devices = (1 << 3),
    FootprintPads = (1 << 4),
    Planes = (1 << 5),
    Zones = (1 << 6),
    Polygons = (1 << 7),
    StrokeTexts = (1 << 8),
    Holes = (1 << 9),
    All = Vias | NetPoints | NetLines | Devices | FootprintPads | Planes |
        Zones | Polygons | StrokeTexts | Holes,

    // Match behavior
    AcceptNearMatch = (1 << 10),
    AcceptNextGridMatch = (1 << 11),

    // Performance options
    SkipLowerPriorityMatches = (1 << 15),

    // Other options
    DevicesOfPads = (1 << 20),  // Don't return pads, but their device.
  };
  Q_DECLARE_FLAGS(FindFlags, FindFlag)

  // Constructors / Destructor
  BoardEditorState() = delete;
  BoardEditorState(const BoardEditorState& other) = delete;
  explicit BoardEditorState(const Context& context,
                            QObject* parent = nullptr) noexcept;
  virtual ~BoardEditorState() noexcept;

  // General Methods
  virtual bool entry() noexcept { return true; }
  virtual bool exit() noexcept { return true; }

  // Event Handlers
  virtual bool processAddDevice(ComponentInstance& component,
                                const Uuid& device,
                                const Uuid& footprint) noexcept {
    Q_UNUSED(component);
    Q_UNUSED(device);
    Q_UNUSED(footprint);
    return false;
  }
  virtual bool processImportDxf() noexcept { return false; }
  virtual bool processSelectAll() noexcept { return false; }
  virtual bool processCut() noexcept { return false; }
  virtual bool processCopy() noexcept { return false; }
  virtual bool processPaste() noexcept { return false; }
  virtual bool processMove(const Point& delta) noexcept {
    Q_UNUSED(delta);
    return false;
  }
  virtual bool processRotate(const Angle& rotation) noexcept {
    Q_UNUSED(rotation);
    return false;
  }
  virtual bool processFlip(Qt::Orientation orientation) noexcept {
    Q_UNUSED(orientation);
    return false;
  }
  virtual bool processSnapToGrid() noexcept { return false; }
  virtual bool processSetLocked(bool locked) noexcept {
    Q_UNUSED(locked);
    return false;
  }
  virtual bool processChangeLineWidth(int step) noexcept {
    Q_UNUSED(step);
    return false;
  }
  virtual bool processResetAllTexts() noexcept { return false; }
  virtual bool processRemove() noexcept { return false; }
  virtual bool processEditProperties() noexcept { return false; }
  virtual bool processAbortCommand() noexcept { return false; }
  virtual bool processKeyPressed(const GraphicsSceneKeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processKeyReleased(const GraphicsSceneKeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }

  // Operator Overloadings
  BoardEditorState& operator=(const BoardEditorState& rhs) = delete;

signals:
  /**
   * @brief Signal to indicate that the current tool should be exited
   *
   * This signal can be emitted by each state to signalize the FSM to leave
   * the current state and entering the select tool.
   */
  void requestLeavingState();

protected:  // Methods
  BoardGraphicsScene* getActiveBoardScene() noexcept;
  bool getIgnoreLocks() const noexcept;
  PositiveLength getGridInterval() const noexcept;
  const LengthUnit& getLengthUnit() const noexcept;
  QSet<const Layer*> getAllowedGeometryLayers() noexcept;
  void makeLayerVisible(const QString& layer) noexcept;
  void abortBlockingToolsInOtherEditors() noexcept;
  bool execCmd(UndoCommand* cmd);
  QWidget* parentWidget() noexcept;
  QList<std::shared_ptr<QGraphicsItem>> findItemsAtPos(
      const Point& pos, FindFlags flags, const Layer* cuLayer = nullptr,
      const QSet<const NetSignal*>& netsignals = {},
      const QVector<std::shared_ptr<QGraphicsItem>>& except = {}) noexcept;
  template <typename T = QGraphicsItem>
  std::shared_ptr<T> findItemAtPos(
      const Point& pos, FindFlags flags, const Layer* cuLayer = nullptr,
      const QSet<const NetSignal*>& netsignals = {},
      const QVector<std::shared_ptr<QGraphicsItem>>& except = {}) noexcept {
    const QList<std::shared_ptr<QGraphicsItem>> items =
        findItemsAtPos(pos, flags | FindFlag::SkipLowerPriorityMatches, cuLayer,
                       netsignals, except);
    std::shared_ptr<T> castedItem =
        std::dynamic_pointer_cast<T>(items.value(0, nullptr));
    if ((!items.isEmpty()) && (!castedItem)) {
      // Probably wrong flags are passed?!?!
      qCritical() << "Found a board item, but it has the wrong type!";
    }
    return castedItem;
  }

protected:  // Data
  Context mContext;
  BoardEditorFsmAdapter& mAdapter;
};

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::BoardEditorState::FindFlags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
