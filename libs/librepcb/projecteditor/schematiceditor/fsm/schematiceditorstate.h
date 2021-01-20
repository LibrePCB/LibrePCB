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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORSTATE_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORSTATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorfsm.h"

#include <librepcb/common/units/all_length_units.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoCommand;
class GraphicsLayer;

namespace project {

class Schematic;

namespace editor {

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
  virtual bool processAddComponent() noexcept { return false; }
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
  virtual bool processRotateCw() noexcept { return false; }
  virtual bool processRotateCcw() noexcept { return false; }
  virtual bool processMirror() noexcept { return false; }
  virtual bool processRemove() noexcept { return false; }
  virtual bool processAbortCommand() noexcept { return false; }
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneLeftMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processSwitchToSchematicPage(int index) noexcept {
    Q_UNUSED(index);
    return false;  // Do NOT allow switching page by default
  }

  // Operator Overloadings
  SchematicEditorState& operator=(const SchematicEditorState& rhs) = delete;

protected:  // Methods
  Schematic* getActiveSchematic() noexcept;
  PositiveLength getGridInterval() const noexcept;
  const LengthUnit& getDefaultLengthUnit() const noexcept;
  QList<GraphicsLayer*> getAllowedGeometryLayers() const noexcept;
  bool execCmd(UndoCommand* cmd);
  QWidget* parentWidget() noexcept;

protected:  // Data
  Context mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
