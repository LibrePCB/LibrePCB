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
#include "spacemousesettingswidget.h"

#include "ui_spacemousesettingswidget.h"

#include <QtCore>
#include <QtWidgets>

#include <cmath>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/
namespace {

// The value the sensitivity sliders report is used as an *exponent* in the
// sensitivity multiplier calculation.  Slider values in the range of
// [-kSliderRange, +kSliderRange] are first scaled by kSliderRange to result
// in a final exponent in the range of [-1.0, 1.0].  The final multiplier
// value is obtained from (kSensitivityCurveBase)^exponent. This gives the
// sliders a more usable feel than a linear mapping would, while
// simultaneously keeping the nominal 1.0x multiplier in the middle.

// Base of the exponential slider-to-sensitivity curve. This results in
// a multiplier range of 1/n..n (where n is the base) at full deflection.
constexpr double kSensitivityCurveBase = 3.0;

// Slider range: each sensitivity slider spans [-kSliderRange, +kSliderRange],
// and is normalized to [-1.0, 1.0].  kSliderRange thus controls the
// granularity of the sensitivity setting.
constexpr int kSliderRange = 100;  // Multipliers will be in hundredths

// Convert a Space Mouse sensitivity slider position to a sensitivity
// multiplier. See the block comment above for the curve this implements.
double sliderToSensitivity(int sliderValue) noexcept {
  return std::pow(kSensitivityCurveBase,
                  sliderValue / static_cast<double>(kSliderRange));
}

// Inverse of ::sliderToSensitivity().
int sensitivityToSlider(double sensitivity) noexcept {
  if (sensitivity <= 0) {
    // Not a valid sensitivity (shouldn't normally happen since the UI can
    // only produce values > 0) - fall back to nominal (1.0x, slider at 0).
    return 0;
  }
  return qBound(-kSliderRange,
                qRound(kSliderRange * std::log(sensitivity) /
                       std::log(kSensitivityCurveBase)),
                kSliderRange);
}

// Format a sensitivity multiplier for display (e.g. "1.2x").
[[maybe_unused]] QString sensitivityLabel(double sensitivity) noexcept {
  return QString("%1x").arg(sensitivity, 0, 'f', 1);
}

}  // namespace

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SpaceMouseSettingsWidget::SpaceMouseSettingsWidget(
    WorkspaceSettingsItem_SpaceMouse& settings, QWidget* parent) noexcept
  : QWidget(parent),
    mSettings(settings),
    mUi(new Ui::SpaceMouseSettingsWidget) {
  mUi->setupUi(this);

  using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;
  mAxes = {
      {Axis::TranslationX, mUi->lblSpaceMouseIconPanH,
       ":/img/settings/spacemouse-panx.svg", mUi->sldSpaceMousePanH,
       mUi->lblSpaceMousePanHValue, mUi->chkSpaceMousePanHInvert},
      {Axis::TranslationY, mUi->lblSpaceMouseIconPanV,
       ":/img/settings/spacemouse-pany.svg", mUi->sldSpaceMousePanV,
       mUi->lblSpaceMousePanVValue, mUi->chkSpaceMousePanVInvert},
      {Axis::TranslationZ, mUi->lblSpaceMouseIconZoom,
       ":/img/settings/spacemouse-panz.svg", mUi->sldSpaceMouseZoom,
       mUi->lblSpaceMouseZoomValue, mUi->chkSpaceMouseZoomInvert},
      {Axis::RotationX, mUi->lblSpaceMouseIconPitch,
       ":/img/settings/spacemouse-pitch.svg", mUi->sldSpaceMousePitch,
       mUi->lblSpaceMousePitchValue, mUi->chkSpaceMousePitchInvert},
      {Axis::RotationY, mUi->lblSpaceMouseIconRoll,
       ":/img/settings/spacemouse-roll.svg", mUi->sldSpaceMouseRoll,
       mUi->lblSpaceMouseRollValue, mUi->chkSpaceMouseRollInvert},
      {Axis::RotationZ, mUi->lblSpaceMouseIconYaw,
       ":/img/settings/spacemouse-yaw.svg", mUi->sldSpaceMouseYaw,
       mUi->lblSpaceMouseYawValue, mUi->chkSpaceMouseYawInvert},
  };
  for (const AxisWidgets& w : std::as_const(mAxes)) {
    w.icon->setPixmap(QIcon(w.iconPath).pixmap(20, 20));
    // The raw slider value is converted into a sensitivity multiplier
    // prior to being displayed by the label.
    auto updateLabel = [w](int value) {
      w.valueLabel->setText(sensitivityLabel(sliderToSensitivity(value)));
    };
    updateLabel(w.slider->value());
    connect(w.slider, &QSlider::valueChanged, w.valueLabel, updateLabel);
    w.slider->installEventFilter(this);
  }

  load();
}

SpaceMouseSettingsWidget::~SpaceMouseSettingsWidget() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SpaceMouseSettingsWidget::load() noexcept {
  // Setting the slider value also updates the value label (see constructor).
  for (const AxisWidgets& w : std::as_const(mAxes)) {
    const WorkspaceSettingsItem_SpaceMouse::AxisSettings& s =
        mSettings.get(w.axis);
    w.slider->setValue(sensitivityToSlider(s.sensitivity));
    w.invert->setChecked(s.invert);
  }
}

void SpaceMouseSettingsWidget::save() noexcept {
  WorkspaceSettingsItem_SpaceMouse::AxisSettingsMap settings;
  for (const AxisWidgets& w : std::as_const(mAxes)) {
    WorkspaceSettingsItem_SpaceMouse::AxisSettings s;
    s.sensitivity = sliderToSensitivity(w.slider->value());
    s.invert = w.invert->isChecked();
    settings[w.axis] = s;
  }
  if (!settings.isEmpty()) {
    mSettings.set(settings);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SpaceMouseSettingsWidget::eventFilter(QObject* watched,
                                           QEvent* event) noexcept {
  // Double-clicking a Space Mouse sensitivity slider resets it to nominal
  // (1.0x, slider position 0) - QSlider has no dedicated double-click
  // signal of its own, so we use an event filter to watch for it. The filter
  // is only installed on the sensitivity sliders.
  if (event->type() == QEvent::MouseButtonDblClick) {
    if (QSlider* slider = qobject_cast<QSlider*>(watched)) {
      slider->setValue(0);
      // Consume the event so QSlider doesn't act on it as well.
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
