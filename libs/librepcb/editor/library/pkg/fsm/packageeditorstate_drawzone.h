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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWZONE_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWZONE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CmdZoneEdit;
class ZoneGraphicsItem;

/*******************************************************************************
 *  Class PackageEditorState_DrawZone
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawZone class
 */
class PackageEditorState_DrawZone : public PackageEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_DrawZone() = delete;
  PackageEditorState_DrawZone(const PackageEditorState_DrawZone& other) =
      delete;
  explicit PackageEditorState_DrawZone(Context& context) noexcept;
  virtual ~PackageEditorState_DrawZone() noexcept;

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
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  PackageEditorState_DrawZone& operator=(
      const PackageEditorState_DrawZone& rhs) = delete;

private:  // Methods
  bool start() noexcept;
  bool abort(bool showErrMsgBox = true) noexcept;
  bool addNextSegment() noexcept;
  void updateCursorPosition(Qt::KeyboardModifiers modifiers) noexcept;
  void updateOutline() noexcept;
  void updateOverlayText() noexcept;
  void updateStatusBarMessage() noexcept;
  void angleEditValueChanged(const Angle& value) noexcept;

private:  // Types / Data
  bool mIsUndoCmdActive;
  QScopedPointer<CmdZoneEdit> mEditCmd;
  std::shared_ptr<Zone> mCurrentZone;
  std::shared_ptr<ZoneGraphicsItem> mCurrentGraphicsItem;
  Point mLastScenePos;
  Point mCursorPos;

  // Parameter memory
  Zone::Layers mLastLayers;
  Zone::Rules mLastRules;
  Angle mLastAngle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
