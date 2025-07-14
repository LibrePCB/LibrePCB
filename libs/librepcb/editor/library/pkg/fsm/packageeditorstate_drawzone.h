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

#include <QtCore>

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

  // Event Handlers
  bool processKeyPressed(const GraphicsSceneKeyEvent& e) noexcept override;
  bool processKeyReleased(const GraphicsSceneKeyEvent& e) noexcept override;
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Connection to UI
  Zone::Layers getLayers() const noexcept {
    return mCurrentProperties.getLayers();
  }
  void setLayer(Zone::Layer layer, bool enable) noexcept;
  Zone::Rules getRules() const noexcept {
    return mCurrentProperties.getRules();
  }
  void setRule(Zone::Rule rule, bool enable) noexcept;
  const Angle& getAngle() const noexcept { return mLastAngle; }
  void setAngle(const Angle& angle) noexcept;

  // Operator Overloadings
  PackageEditorState_DrawZone& operator=(
      const PackageEditorState_DrawZone& rhs) = delete;

signals:
  void layersChanged(Zone::Layers layers);
  void rulesChanged(Zone::Rules rules);
  void angleChanged(const Angle& angle);

private:  // Methods
  bool start() noexcept;
  bool abort(bool showErrMsgBox = true) noexcept;
  bool addNextSegment() noexcept;
  void updateCursorPosition(Qt::KeyboardModifiers modifiers) noexcept;
  void updateOutline() noexcept;
  void updateOverlayText() noexcept;
  void updateStatusBarMessage() noexcept;

private:
  Point mLastScenePos;
  Angle mLastAngle;
  Point mCursorPos;
  bool mIsUndoCmdActive;

  // Current tool settings
  Zone mCurrentProperties;

  // Information about the current zone to draw. Only valid if
  // mIsUndoCmdActive == true.
  std::shared_ptr<Zone> mCurrentZone;
  std::shared_ptr<ZoneGraphicsItem> mCurrentGraphicsItem;
  std::unique_ptr<CmdZoneEdit> mCurrentEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
