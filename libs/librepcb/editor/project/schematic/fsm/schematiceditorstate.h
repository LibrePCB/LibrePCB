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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicsscene.h"
#include "schematiceditorfsm.h"
#include "schematiceditorfsmadapter.h"

#include <librepcb/core/types/length.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class LengthUnit;
class Point;
class Schematic;

namespace editor {

class SchematicGraphicsScene;
class UndoCommand;

/*******************************************************************************
 *  Class SchematicEditorState
 ******************************************************************************/

/**
 * @brief The schematic editor state base class
 */
class SchematicEditorState : public QObject {
  Q_OBJECT

public:
  using Context = SchematicEditorFsm::Context;

  enum class FindFlag {
    // Item types
    NetPoints = (1 << 0),
    NetLines = (1 << 1),
    NetLabels = (1 << 2),
    Symbols = (1 << 3),
    SymbolPins = (1 << 4),
    SymbolPinsWithComponentSignal = (1 << 5),  // Subset of SymbolPins.
    Polygons = (1 << 6),
    Texts = (1 << 7),
    All = NetPoints | NetLines | NetLabels | Symbols | SymbolPins | Polygons |
        Texts,

    // Match behavior
    AcceptNearMatch = (1 << 10),
    AcceptNearestWithinGrid = (1 << 11),

    // Performance options
    SkipLowerPriorityMatches = (1 << 15),
  };
  Q_DECLARE_FLAGS(FindFlags, FindFlag)

  // Constructors / Destructor
  SchematicEditorState() = delete;
  SchematicEditorState(const SchematicEditorState& other) = delete;
  explicit SchematicEditorState(const Context& context,
                                QObject* parent = nullptr) noexcept;
  virtual ~SchematicEditorState() noexcept;

  // General Methods
  virtual bool entry() noexcept { return true; }
  virtual bool exit() noexcept { return true; }

  // Event Handlers
  virtual bool processAddComponent(
      const QString& searchTerm = QString()) noexcept {
    Q_UNUSED(searchTerm);
    return false;
  }
  virtual bool processAddComponent(const Uuid& cmp,
                                   const Uuid& symbVar) noexcept {
    Q_UNUSED(cmp);
    Q_UNUSED(symbVar);
    return false;
  }
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
  virtual bool processMirror(Qt::Orientation orientation) noexcept {
    Q_UNUSED(orientation);
    return false;
  }
  virtual bool processSnapToGrid() noexcept { return false; }
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
  SchematicEditorState& operator=(const SchematicEditorState& rhs) = delete;

signals:
  /**
   * @brief Signal to indicate that the current tool should be exited
   *
   * This signal can be emitted by each state to signalize the FSM to leave
   * the current state and entering the select tool.
   */
  void requestLeavingState();

protected:  // Methods
  SchematicGraphicsScene* getActiveSchematicScene() noexcept;
  PositiveLength getGridInterval() const noexcept;
  const LengthUnit& getLengthUnit() const noexcept;
  static const QSet<const Layer*>& getAllowedGeometryLayers() noexcept;
  void abortBlockingToolsInOtherEditors() noexcept;
  bool execCmd(UndoCommand* cmd);
  QWidget* parentWidget() noexcept;
  QList<std::shared_ptr<QGraphicsItem>> findItemsAtPos(
      const Point& pos, FindFlags flags,
      const QVector<std::shared_ptr<QGraphicsItem>>& except = {}) noexcept;
  template <typename T = QGraphicsItem>
  std::shared_ptr<T> findItemAtPos(
      const Point& pos, FindFlags flags,
      const QVector<std::shared_ptr<QGraphicsItem>>& except = {}) noexcept {
    const QList<std::shared_ptr<QGraphicsItem>> items =
        findItemsAtPos(pos, flags | FindFlag::SkipLowerPriorityMatches, except);
    std::shared_ptr<T> castedItem =
        std::dynamic_pointer_cast<T>(items.value(0, nullptr));
    if ((!items.isEmpty()) && (!castedItem)) {
      // Probably wrong flags are passed?!?!
      qCritical() << "Found a schematic item, but it has the wrong type!";
    }
    return castedItem;
  }

protected:  // Data
  Context mContext;
  SchematicEditorFsmAdapter& mAdapter;
};

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::SchematicEditorState::FindFlags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
