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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspacesettingsitem_spacemouse.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

namespace {

// Decimal digits of precision used when serializing a Space Mouse axis
// sensitivity multiplier to the settings file.  Since multipliers are
// presently in units of 1/100, six decimal places is overkill.
// (see kSliderRange in spacemousesettingswidget.cpp)
constexpr int kSensitivitySerializationDecimals = 6;

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
    s.sensitivity = child->getChild("sensitivity/@0").getValue().toDouble();
    s.invert = deserialize<bool>(child->getChild("invert/@0"));
    settings[*axis] = s;
  }

  if (settings != mAxisSettings) {
    mAxisSettings = settings;
    valueModified();
  }
}

void WorkspaceSettingsItem_SpaceMouse::serializeImpl(SExpression& root) const {
  // Iterate in a fixed, deterministic order for a clean file format.
  const QList<Axis> orderedAxes = {
      Axis::TranslationX, Axis::TranslationY, Axis::TranslationZ,
      Axis::RotationX,    Axis::RotationY,    Axis::RotationZ,
  };
  foreach (Axis axis, orderedAxes) {
    const AxisSettings& s = mAxisSettings.value(axis);
    root.ensureLineBreak();
    SExpression& child = root.appendList("axis");
    child.appendChild(SExpression::createToken(axisToString(axis)));
    child.appendChild("sensitivity",
        QString::number(s.sensitivity, 'f', kSensitivitySerializationDecimals));
    child.appendChild("invert", s.invert);
  }
  root.ensureLineBreak();
}

QString WorkspaceSettingsItem_SpaceMouse::axisToString(Axis axis) noexcept {
  switch (axis) {
    case Axis::TranslationX:
      return "translation_x";
    case Axis::TranslationY:
      return "translation_y";
    case Axis::TranslationZ:
      return "translation_z";
    case Axis::RotationX:
      return "rotation_x";
    case Axis::RotationY:
      return "rotation_y";
    case Axis::RotationZ:
      return "rotation_z";
    default:
      return QString();
  }
}

std::optional<WorkspaceSettingsItem_SpaceMouse::Axis>
    WorkspaceSettingsItem_SpaceMouse::axisFromString(
        const QString& str) noexcept {
  if (str == QLatin1String("translation_x")) return Axis::TranslationX;
  if (str == QLatin1String("translation_y")) return Axis::TranslationY;
  if (str == QLatin1String("translation_z")) return Axis::TranslationZ;
  if (str == QLatin1String("rotation_x")) return Axis::RotationX;
  if (str == QLatin1String("rotation_y")) return Axis::RotationY;
  if (str == QLatin1String("rotation_z")) return Axis::RotationZ;
  return std::nullopt;
}

WorkspaceSettingsItem_SpaceMouse::AxisSettingsMap
    WorkspaceSettingsItem_SpaceMouse::defaultAxisSettings() noexcept {
  AxisSettingsMap defaults;
  defaults[Axis::TranslationX] = AxisSettings();
  defaults[Axis::TranslationY] = AxisSettings();
  defaults[Axis::TranslationZ] = AxisSettings();
  defaults[Axis::RotationX] = AxisSettings();
  defaults[Axis::RotationY] = AxisSettings();
  defaults[Axis::RotationZ] = AxisSettings();
  return defaults;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
