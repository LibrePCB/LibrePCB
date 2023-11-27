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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../utils/toolbarproxy.h"
#include "symboleditorfsm.h"

#include <librepcb/core/types/length.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class LengthUnit;

namespace editor {

/*******************************************************************************
 *  Class SymbolEditorState
 ******************************************************************************/

/**
 * @brief The SymbolEditorState class is the base class of all symbol editor FSM
 * states
 */
class SymbolEditorState : public QObject {
  Q_OBJECT

public:
  using Context = SymbolEditorFsm::Context;

  // Constructors / Destructor
  SymbolEditorState() = delete;
  SymbolEditorState(const SymbolEditorState& other) = delete;
  explicit SymbolEditorState(const Context& context) noexcept;
  virtual ~SymbolEditorState() noexcept;

  // General Methods
  virtual bool entry() noexcept { return true; }
  virtual bool exit() noexcept { return true; }
  virtual QSet<EditorWidgetBase::Feature> getAvailableFeatures()
      const noexcept = 0;

  // Event Handlers
  virtual bool processKeyPressed(const QKeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processKeyReleased(const QKeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
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
  virtual bool processSelectAll() noexcept { return false; }
  virtual bool processCut() noexcept { return false; }
  virtual bool processCopy() noexcept { return false; }
  virtual bool processPaste() noexcept { return false; }
  virtual bool processMove(Qt::ArrowType direction) {
    Q_UNUSED(direction);
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
  virtual bool processRemove() noexcept { return false; }
  virtual bool processEditProperties() noexcept { return false; }
  virtual bool processImportDxf() noexcept { return false; }
  virtual bool processAbortCommand() noexcept { return false; }

  // Operator Overloadings
  SymbolEditorState& operator=(const SymbolEditorState& rhs) = delete;

signals:
  void availableFeaturesChanged();
  void statusBarMessageChanged(const QString& message, int timeoutMs = -1);

protected:  // Methods
  const PositiveLength& getGridInterval() const noexcept;
  const LengthUnit& getLengthUnit() const noexcept;
  static const QSet<const Layer*>& getAllowedTextLayers() noexcept;
  static const QSet<const Layer*>& getAllowedCircleAndPolygonLayers() noexcept;

protected:  // Data
  Context mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
