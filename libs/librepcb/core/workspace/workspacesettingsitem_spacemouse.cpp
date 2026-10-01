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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspacesettingsitem_spacemouse.h"

#include <QtCore>

#include <cmath>
#include <optional>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

namespace {

// Decimal digits of precision used when serializing a Space Mouse axis
// sensitivity multiplier to the settings file. Since the settings UI only
// offers multipliers in steps of 1/100, six decimal places is plenty.
constexpr int kSensitivitySerializationDecimals = 6;

using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;

// All axes with their identifiers in the settings file. The order of this
// table is also the (fixed) order in which axes are serialized.
struct AxisInfo {
  Axis axis;
  const char* name;
};
constexpr AxisInfo kAxes[] = {
    {Axis::TranslationX, "translation_x"},
    {Axis::TranslationY, "translation_y"},
    {Axis::TranslationZ, "translation_z"},
    {Axis::RotationX, "rotation_x"},
    {Axis::RotationY, "rotation_y"},
    {Axis::RotationZ, "rotation_z"},
};

std::optional<Axis> axisFromString(const QString& str) noexcept {
  for (const AxisInfo& info : kAxes) {
    if (str == QLatin1String(info.name)) {
      return info.axis;
    }
  }
  return std::nullopt;
}

}  // namespace

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettingsItem_SpaceMouse::WorkspaceSettingsItem_SpaceMouse(
    QObject* parent) noexcept
  : WorkspaceSettingsItem("spacemouse", parent),
    mAxisSettings(defaultAxisSettings()) {
}

WorkspaceSettingsItem_SpaceMouse::~WorkspaceSettingsItem_SpaceMouse() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

const WorkspaceSettingsItem_SpaceMouse::AxisSettings&
    WorkspaceSettingsItem_SpaceMouse::get(Axis axis) const noexcept {
  auto it = mAxisSettings.find(axis);
  if (it != mAxisSettings.end()) {
    return it.value();
  }
  static const AxisSettings sDefault;
  return sDefault;
}

void WorkspaceSettingsItem_SpaceMouse::set(
    Axis axis, const AxisSettings& settings) noexcept {
  if (mAxisSettings.value(axis) != settings) {
    mAxisSettings[axis] = settings;
    valueModified();
  }
}

void WorkspaceSettingsItem_SpaceMouse::set(
    const AxisSettingsMap& settings) noexcept {
  AxisSettingsMap newSettings = defaultAxisSettings();
  for (auto it = settings.begin(); it != settings.end(); it++) {
    if (newSettings.contains(it.key())) {
      newSettings[it.key()] = it.value();
    }
  }
  if (newSettings != mAxisSettings) {
    mAxisSettings = newSettings;
    valueModified();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WorkspaceSettingsItem_SpaceMouse::restoreDefaultImpl() noexcept {
  const AxisSettingsMap defaults = defaultAxisSettings();
  if (mAxisSettings != defaults) {
    mAxisSettings = defaults;
    valueModified();
  }
}

void WorkspaceSettingsItem_SpaceMouse::loadImpl(const SExpression& root) {
  // Temporary objects to make this method atomic.
  AxisSettingsMap settings = defaultAxisSettings();
  foreach (const SExpression* child, root.getChildren("axis")) {
    const std::optional<Axis> axis =
        axisFromString(child->getChild("@0").getValue());
    if (!axis) {
      continue;  // Unknown axis identifier, ignore (e.g. future file format).
    }
    AxisSettings s;
    // Ignore invalid values (zero, negative, NaN, ...) to avoid ending up
    // with a dead axis due to a corrupted settings file.
    bool ok = false;
    const double sensitivity =
        child->getChild("sensitivity/@0").getValue().toDouble(&ok);
    if (ok && std::isfinite(sensitivity) && (sensitivity > 0)) {
      s.sensitivity = sensitivity;
    }
    s.invert = deserialize<bool>(child->getChild("invert/@0"));
    settings[*axis] = s;
  }

  if (settings != mAxisSettings) {
    mAxisSettings = settings;
    valueModified();
  }
}

void WorkspaceSettingsItem_SpaceMouse::serializeImpl(SExpression& root) const {
  for (const AxisInfo& info : kAxes) {
    const AxisSettings& s = mAxisSettings.value(info.axis);
    root.ensureLineBreak();
    SExpression& child = root.appendList("axis");
    child.appendChild(SExpression::createToken(info.name));
    child.appendChild(
        "sensitivity",
        QString::number(s.sensitivity, 'f', kSensitivitySerializationDecimals));
    child.appendChild("invert", s.invert);
  }
  root.ensureLineBreak();
}

WorkspaceSettingsItem_SpaceMouse::AxisSettingsMap
    WorkspaceSettingsItem_SpaceMouse::defaultAxisSettings() noexcept {
  AxisSettingsMap defaults;
  for (const AxisInfo& info : kAxes) {
    defaults[info.axis] = AxisSettings();
  }
  return defaults;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
