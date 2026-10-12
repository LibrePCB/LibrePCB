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
 * @brief Widget (GUI) to view and modify the SpaceMouse settings
 *
 * Shown as the "Space Mouse" tab of WorkspaceSettingsDialog. Changes are
 * only applied to the settings object by calling save().
 */
class SpaceMouseSettingsWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  SpaceMouseSettingsWidget() = delete;
  SpaceMouseSettingsWidget(const SpaceMouseSettingsWidget& other) = delete;
  explicit SpaceMouseSettingsWidget(WorkspaceSettingsItem_SpaceMouse& settings,
                                    QWidget* parent = nullptr) noexcept;
  ~SpaceMouseSettingsWidget() noexcept override;

  // General Methods

  /**
   * @brief Reload all widgets from the settings object
   */
  void load() noexcept;

  /**
   * @brief Write the state of all widgets to the settings object
   */
  void save() noexcept;

  // Operator Overloadings
  SpaceMouseSettingsWidget& operator=(const SpaceMouseSettingsWidget& rhs) =
      delete;

private:  // Types
  /// The widgets belonging to one motion axis
  struct AxisWidgets {
    WorkspaceSettingsItem_SpaceMouse::Axis axis;
    QLabel* icon;
    QString iconPath;
    QSlider* slider;
    QLabel* valueLabel;
    QCheckBox* invert;
  };

private:  // Methods
  bool eventFilter(QObject* watched, QEvent* event) noexcept override;

  // Maps a slider position to a sensitivity multiplier.
  static double sliderToSensitivity(int sliderValue) noexcept;

  // Inverse of sliderToSensitivity().
  static int sensitivityToSlider(double sensitivity) noexcept;

  // Formats a multiplier for display, e.g. "1.2x".
  static QString sensitivityLabel(double sensitivity) noexcept;

private:  // Data
  WorkspaceSettingsItem_SpaceMouse& mSettings;
  QScopedPointer<Ui::SpaceMouseSettingsWidget> mUi;
  QVector<AxisWidgets> mAxes;

  // Static Variables

  // Slider positions in [-sSliderRange, +sSliderRange] map to a multiplier of
  // sSensitivityCurveBase^(position / sSliderRange), which keeps 1.0x centered.

  // Multiplier range at full deflection is 1/base..base.
  static constexpr double sSensitivityCurveBase = 3.0;

  // Slider extent in each direction. Sets the granularity (hundredths).
  static constexpr int sSliderRange = 100;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
