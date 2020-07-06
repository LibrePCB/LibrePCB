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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorfsm.h"

#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/utils/toolbarproxy.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class PackageEditorState
 ******************************************************************************/

/**
 * @brief The PackageEditorState class is the base class of all package editor
 * FSM states
 */
class PackageEditorState : public QObject {
  Q_OBJECT

public:
  using Context = PackageEditorFsm::Context;

  // Constructors / Destructor
  PackageEditorState()                                = delete;
  PackageEditorState(const PackageEditorState& other) = delete;
  explicit PackageEditorState(Context& context) noexcept;
  virtual ~PackageEditorState() noexcept;

  // General Methods
  virtual bool entry() noexcept { return true; }
  virtual bool exit() noexcept { return true; }

  // Event Handlers
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
  virtual bool processRotateCw() noexcept { return false; }
  virtual bool processRotateCcw() noexcept { return false; }
  virtual bool processMirror() noexcept { return false; }
  virtual bool processFlip() noexcept { return false; }
  virtual bool processRemove() noexcept { return false; }
  virtual bool processAbortCommand() noexcept { return false; }

  // Operator Overloadings
  PackageEditorState& operator=(const PackageEditorState& rhs) = delete;

protected:  // Methods
  const PositiveLength& getGridInterval() const noexcept;
  const LengthUnit&     getDefaultLengthUnit() const noexcept;

protected:  // Data
  Context& mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_H
