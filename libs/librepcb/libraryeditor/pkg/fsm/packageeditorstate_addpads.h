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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_ADDPADS_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_ADDPADS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/library/pkg/footprintpad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class PackagePad;
class CmdFootprintPadEdit;

namespace editor {

class PackagePadComboBox;

/*******************************************************************************
 *  Class PackageEditorState_AddPads
 ******************************************************************************/

/**
 * @brief The PackageEditorState_AddPads class
 */
class PackageEditorState_AddPads : public PackageEditorState {
  Q_OBJECT

public:
  // Types
  enum class PadType { THT, SMT };

  // Constructors / Destructor
  PackageEditorState_AddPads() = delete;
  PackageEditorState_AddPads(const PackageEditorState_AddPads& other) = delete;
  explicit PackageEditorState_AddPads(Context& context, PadType type) noexcept;
  virtual ~PackageEditorState_AddPads() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processRotateCw() noexcept override;
  virtual bool processRotateCcw() noexcept override;

  // Operator Overloadings
  PackageEditorState_AddPads& operator=(const PackageEditorState_AddPads& rhs) =
      delete;

private:  // Methods
  bool startAddPad(const Point& pos) noexcept;
  bool finishAddPad(const Point& pos) noexcept;
  bool abortAddPad() noexcept;
  void selectNextFreePadInComboBox() noexcept;
  void packagePadComboBoxCurrentPadChanged(tl::optional<Uuid> pad) noexcept;
  void boardSideSelectorCurrentSideChanged(
      FootprintPad::BoardSide side) noexcept;
  void shapeSelectorCurrentShapeChanged(FootprintPad::Shape shape) noexcept;
  void widthEditValueChanged(const PositiveLength& value) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;
  void drillDiameterEditValueChanged(const UnsignedLength& value) noexcept;

private:  // Types / Data
  PadType mPadType;
  QScopedPointer<CmdFootprintPadEdit> mEditCmd;
  std::shared_ptr<FootprintPad> mCurrentPad;
  FootprintPadGraphicsItem* mCurrentGraphicsItem;
  PackagePadComboBox* mPackagePadComboBox;

  // parameter memory
  FootprintPad mLastPad;
};

/*******************************************************************************
 *  Class PackageEditorState_AddPadsTht
 ******************************************************************************/

/**
 * @brief The PackageEditorState_AddPadsTht class
 */
class PackageEditorState_AddPadsTht final : public PackageEditorState_AddPads {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_AddPadsTht() = delete;
  PackageEditorState_AddPadsTht(const PackageEditorState_AddPadsTht& other) =
      delete;
  explicit PackageEditorState_AddPadsTht(Context& context) noexcept
    : PackageEditorState_AddPads(context, PadType::THT) {}
  ~PackageEditorState_AddPadsTht() noexcept {}

  // Operator Overloadings
  PackageEditorState_AddPadsTht& operator=(
      const PackageEditorState_AddPadsTht& rhs) = delete;
};

/*******************************************************************************
 *  Class PackageEditorState_AddPadsSmt
 ******************************************************************************/

/**
 * @brief The PackageEditorState_AddPadsSmt class
 */
class PackageEditorState_AddPadsSmt final : public PackageEditorState_AddPads {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_AddPadsSmt() = delete;
  PackageEditorState_AddPadsSmt(const PackageEditorState_AddPadsSmt& other) =
      delete;
  explicit PackageEditorState_AddPadsSmt(Context& context) noexcept
    : PackageEditorState_AddPads(context, PadType::SMT) {}
  ~PackageEditorState_AddPadsSmt() noexcept {}

  // Operator Overloadings
  PackageEditorState_AddPadsSmt& operator=(
      const PackageEditorState_AddPadsSmt& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_ADDPADS_H
