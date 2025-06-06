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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_ADDHOLES_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_ADDHOLES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Hole;

namespace editor {

class CmdHoleEdit;
class HoleGraphicsItem;

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
  QSet<EditorWidgetBase::Feature> getAvailableFeatures()
      const noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  PackageEditorState_AddHoles& operator=(
      const PackageEditorState_AddHoles& rhs) = delete;

private:  // Methods
  bool startAddHole(const Point& pos) noexcept;
  bool finishAddHole(const Point& pos) noexcept;
  bool abortAddHole() noexcept;
  void diameterEditValueChanged(const PositiveLength& value) noexcept;

private:  // Data
  std::unique_ptr<CmdHoleEdit> mEditCmd;
  std::shared_ptr<Hole> mCurrentHole;
  std::shared_ptr<HoleGraphicsItem> mCurrentGraphicsItem;

  // parameter memory
  PositiveLength mLastDiameter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
