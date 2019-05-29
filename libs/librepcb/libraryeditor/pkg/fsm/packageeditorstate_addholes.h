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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_ADDHOLES_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_ADDHOLES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Hole;
class HoleGraphicsItem;
class CmdHoleEdit;

namespace library {

namespace editor {

/*******************************************************************************
 *  Class PackageEditorState_AddHoles
 ******************************************************************************/

/**
 * @brief The PackageEditorState_AddHoles class
 */
class PackageEditorState_AddHoles final : public PackageEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_AddHoles() = delete;
  PackageEditorState_AddHoles(const PackageEditorState_AddHoles& other) =
      delete;
  explicit PackageEditorState_AddHoles(Context& context) noexcept;
  ~PackageEditorState_AddHoles() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  PackageEditorState_AddHoles& operator       =(
      const PackageEditorState_AddHoles& rhs) = delete;

private:  // Methods
  bool startAddHole(const Point& pos) noexcept;
  bool finishAddHole(const Point& pos) noexcept;
  bool abortAddHole() noexcept;
  void diameterSpinBoxValueChanged(double value) noexcept;

private:  // Data
  Point                       mStartPos;
  QScopedPointer<CmdHoleEdit> mEditCmd;
  Hole*                       mCurrentHole;
  HoleGraphicsItem*           mCurrentGraphicsItem;

  // parameter memory
  PositiveLength mLastDiameter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_ADDHOLES_H
