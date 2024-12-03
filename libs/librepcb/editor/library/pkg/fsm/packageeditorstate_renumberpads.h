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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_RENUMBERPADS_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_RENUMBERPADS_H

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

class PackagePad;

namespace editor {

class FootprintPadGraphicsItem;

/*******************************************************************************
 *  Class PackageEditorState_ReNumberPads
 ******************************************************************************/

/**
 * @brief The PackageEditorState_ReNumberPads class
 */
class PackageEditorState_ReNumberPads final : public PackageEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_ReNumberPads() = delete;
  PackageEditorState_ReNumberPads(
      const PackageEditorState_ReNumberPads& other) = delete;
  explicit PackageEditorState_ReNumberPads(Context& context) noexcept;
  ~PackageEditorState_ReNumberPads() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;
  QSet<EditorWidgetBase::Feature> getAvailableFeatures()
      const noexcept override;

  // Event Handlers
  bool processKeyPressed(const QKeyEvent& e) noexcept override;
  bool processKeyReleased(const QKeyEvent& e) noexcept override;
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  PackageEditorState_ReNumberPads& operator=(
      const PackageEditorState_ReNumberPads& rhs) = delete;

private:  // Methods
  bool start() noexcept;
  void updateCurrentPad(bool force = false) noexcept;
  bool commitCurrentPad() noexcept;
  void finish() noexcept;
  int findIndexOfPad(const Uuid& uuid) const noexcept;

private:  // Data
  bool mUndoCmdActive;
  int mAssignedFootprintPadCount;

  QVector<std::shared_ptr<PackagePad>> mPackagePads;

  std::shared_ptr<FootprintPadGraphicsItem> mPreviousPad;
  std::shared_ptr<FootprintPadGraphicsItem> mCurrentPad;
  std::unique_ptr<UndoCommandGroup> mTmpCmd;

  Point mCurrentPos;
  Qt::KeyboardModifiers mCurrentModifiers;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
