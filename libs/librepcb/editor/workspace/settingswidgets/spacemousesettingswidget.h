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

// AI DISCLAIMER: This file was authored and/or modified by Claude AI.
// It was fully reviewed by a human.

#ifndef LIBREPCB_EDITOR_SPACEMOUSESETTINGSWIDGET_H
#define LIBREPCB_EDITOR_SPACEMOUSESETTINGSWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/workspace/workspacesettingsitem_spacemouse.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class SpaceMouseSettingsWidget;
}

/*******************************************************************************
 *  Class SpaceMouseSettingsWidget
 ******************************************************************************/

/**
 * @brief Widget (GUI) to view and modify the Space Mouse settings
 *
 * Shown as the "Space Mouse" tab of ::WorkspaceSettingsDialog. Extracted out
 * of that dialog into its own widget since the Space Mouse controls (six
 * axis sliders/checkboxes, LED toggle, and the "unavailable" fallback page)
 * made up a disproportionate share of the dialog's code.
 *
 * Mirrors the existing `*OutputJobWidget` family in
 * `editor/project/outputjobsdialog/`: constructed directly (no Designer
 * promotion) and dropped into a placeholder container in the parent's
 * `.ui`. Unlike that family, this widget does not write through to its
 * settings object live - Space Mouse settings (like the LED checkbox) are
 * deliberately applied only on the dialog's Apply/OK, so the caller must
 * explicitly call ::save().
 */
class SpaceMouseSettingsWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  SpaceMouseSettingsWidget() = delete;
  SpaceMouseSettingsWidget(const SpaceMouseSettingsWidget& other) = delete;
  explicit SpaceMouseSettingsWidget(
      WorkspaceSettingsItem_SpaceMouse& settings,
      QWidget* parent = nullptr) noexcept;
  ~SpaceMouseSettingsWidget() noexcept override;

  // General Methods

  /**
   * @brief Reload all widgets from the current values of the settings object
   *
   * Called once by the constructor, and callable again if the parent
   * dialog needs to reset the tab back to the persisted values (e.g. when
   * the user discards unsaved changes).
   */
  void load() noexcept;

  /**
   * @brief Write the current state of all widgets back into the settings
   *        object passed to the constructor
   *
   * Not called automatically - the parent dialog decides when settings are
   * actually persisted (on Apply/OK), same as before this widget existed.
   */
  void save() noexcept;

  // Operator Overloadings
  SpaceMouseSettingsWidget& operator=(const SpaceMouseSettingsWidget& rhs) =
      delete;

private:  // Methods
  bool eventFilter(QObject* watched, QEvent* event) noexcept override;

private:  // Data
  WorkspaceSettingsItem_SpaceMouse& mSettings;
  QScopedPointer<Ui::SpaceMouseSettingsWidget> mUi;
  QVector<QSlider*> mSliders;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
