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

// DISCLAIMER: Claude AI assisted in the writing of this file.
// It was reviewed by a human.

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_SPACEMOUSE_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_SPACEMOUSE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspacesettingsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class WorkspaceSettingsItem_SpaceMouse
 ******************************************************************************/

/**
 * @brief Implementation of ::librepcb::WorkspaceSettingsItem to store
 *        SpaceMouse (3Dconnexion/3Dx input device) settings
 *
 * Stores a per-axis sensitivity multiplier and invert flag for each of the
 * six raw motion axes reported by ::librepcb::editor::IF_SpaceMouseInputBackend
 * (see ::librepcb::editor::SpaceMouseMotionEvent).
 */
class WorkspaceSettingsItem_SpaceMouse final : public WorkspaceSettingsItem {
public:
  /**
   * @brief The six SpaceMouse motion axes
   */
  enum class Axis {
    TranslationX,  ///< Horizontal pan (left/right)
    TranslationY,  ///< Vertical pan (away/towards the user)
    TranslationZ,  ///< Zoom (pan/zoom down/up)
    RotationX,  ///< Pitch (rotation about X, in the YZ-plane)
    RotationY,  ///< Roll (rotation about Y, in the XZ-plane)
    RotationZ,  ///< Yaw (rotation about Z, in the XY-plane)
  };

  /**
   * @brief Per-axis settings
   */
  struct AxisSettings {
    double sensitivity = 1.0;  ///< Multiplier, 1.0 = nominal/default rate
    bool invert = false;  ///< Whether to invert the axis direction

    bool operator==(const AxisSettings& rhs) const noexcept {
      return (sensitivity == rhs.sensitivity) && (invert == rhs.invert);
    }
    bool operator!=(const AxisSettings& rhs) const noexcept {
      return !(*this == rhs);
    }
  };

  /**
   * @brief All six axes settings, keyed by ::Axis
   */
  using AxisSettingsMap = QMap<Axis, AxisSettings>;

  // Constructors / Destructor
  WorkspaceSettingsItem_SpaceMouse() = delete;
  WorkspaceSettingsItem_SpaceMouse(
      const WorkspaceSettingsItem_SpaceMouse& other) = delete;
  explicit WorkspaceSettingsItem_SpaceMouse(QObject* parent = nullptr) noexcept;
  ~WorkspaceSettingsItem_SpaceMouse() noexcept override;

  // Getters

  /**
   * @brief Get the settings for a single axis
   *
   * @param axis  The axis to query.
   * @return Sensitivity & invert settings for that axis (defaults if not
   *         explicitly configured).
   */
  const AxisSettings& get(Axis axis) const noexcept;

  /**
   * @brief Get the settings for all six axes
   */
  const AxisSettingsMap& get() const noexcept { return mAxisSettings; }

  // Setters

  /**
   * @brief Set the settings for a single axis
   *
   * @param axis      The axis to modify.
   * @param settings  New sensitivity & invert settings for that axis.
   */
  void set(Axis axis, const AxisSettings& settings) noexcept;

  /**
   * @brief Set the settings for all six axes at once
   *
   * @param settings  New settings, keyed by axis. Axes not contained in the
   *                   map are reset to their default settings.
   */
  void set(const AxisSettingsMap& settings) noexcept;

  // Operator Overloadings
  WorkspaceSettingsItem_SpaceMouse& operator=(
      const WorkspaceSettingsItem_SpaceMouse& rhs) = delete;

private:  // Methods
  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::restoreDefaultImpl()
   */
  void restoreDefaultImpl() noexcept override;

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::loadImpl()
   */
  void loadImpl(const SExpression& root) override;

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::serializeImpl()
   */
  void serializeImpl(SExpression& root) const override;

  static QString axisToString(Axis axis) noexcept;
  static std::optional<Axis> axisFromString(const QString& str) noexcept;
  static AxisSettingsMap defaultAxisSettings() noexcept;

private:
  AxisSettingsMap mAxisSettings;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
