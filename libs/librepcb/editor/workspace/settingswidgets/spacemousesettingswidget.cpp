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
// It has been fully reviewed (and partially edited) by a human.

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

// Convert a Space Mouse sensitivity slider position to a sensitivity
// multiplier.
//
// The slider reports an *exponent* in [-100, 100] (hundredths of [-1.0,
// 1.0]), not the multiplier itself. The actual sensitivity multiplier is
// 3^exponent, which gives a logarithmic ~0.33x..3.0x range while keeping
// the nominal 1.0x exactly centered (slider at 0). This gives the sliders
// a more natural look and feel.
double spaceMouseSliderToSensitivity(int sliderValue) noexcept {
  return std::pow(3.0, sliderValue / 100.0);
}

// Inverse of ::spaceMouseSliderToSensitivity().
int spaceMouseSensitivityToSlider(double sensitivity) noexcept {
  if (sensitivity <= 0) {
    // Not a valid sensitivity (shouldn't normally happen since the UI can
    // only produce values > 0) - fall back to nominal (1.0x, slider at 0).
    return 0;
  }
  return qBound(
      -100, qRound(100.0 * std::log(sensitivity) / std::log(3.0)), 100);
}

// Format a sensitivity multiplier for display (e.g. "1.2x").
QString spaceMouseSensitivityLabel(double sensitivity) noexcept {
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

  // Availability is decided at compile time by LIBREPCB_SPACEMOUSE_AVAILABLE.
#ifdef LIBREPCB_SPACEMOUSE_AVAILABLE
  mUi->stkSpaceMouse->setCurrentWidget(mUi->pageSpaceMouseControls);

  struct SpaceMouseAxisWidgets {
    WorkspaceSettingsItem_SpaceMouse::Axis axis;
    QLabel* icon;
    QString iconPath;
    QSlider* slider;
    QLabel* valueLabel;
    QCheckBox* invert;
  };
  const QVector<SpaceMouseAxisWidgets> axisWidgets = {
      {WorkspaceSettingsItem_SpaceMouse::Axis::TranslationX,
       mUi->lblSpaceMouseIconPanH, ":/img/settings/spacemouse-panx.svg",
       mUi->sldSpaceMousePanH, mUi->lblSpaceMousePanHValue,
       mUi->chkSpaceMousePanHInvert},
      {WorkspaceSettingsItem_SpaceMouse::Axis::TranslationY,
       mUi->lblSpaceMouseIconPanV, ":/img/settings/spacemouse-pany.svg",
       mUi->sldSpaceMousePanV, mUi->lblSpaceMousePanVValue,
       mUi->chkSpaceMousePanVInvert},
      {WorkspaceSettingsItem_SpaceMouse::Axis::TranslationZ,
       mUi->lblSpaceMouseIconZoom, ":/img/settings/spacemouse-panz.svg",
       mUi->sldSpaceMouseZoom, mUi->lblSpaceMouseZoomValue,
       mUi->chkSpaceMouseZoomInvert},
      {WorkspaceSettingsItem_SpaceMouse::Axis::RotationX,
       mUi->lblSpaceMouseIconPitch, ":/img/settings/spacemouse-pitch.svg",
       mUi->sldSpaceMousePitch, mUi->lblSpaceMousePitchValue,
       mUi->chkSpaceMousePitchInvert},
      {WorkspaceSettingsItem_SpaceMouse::Axis::RotationY,
       mUi->lblSpaceMouseIconRoll, ":/img/settings/spacemouse-roll.svg",
       mUi->sldSpaceMouseRoll, mUi->lblSpaceMouseRollValue,
       mUi->chkSpaceMouseRollInvert},
      {WorkspaceSettingsItem_SpaceMouse::Axis::RotationZ,
       mUi->lblSpaceMouseIconYaw, ":/img/settings/spacemouse-yaw.svg",
       mUi->sldSpaceMouseYaw, mUi->lblSpaceMouseYawValue,
       mUi->chkSpaceMouseYawInvert},
  };
  for (const SpaceMouseAxisWidgets& w : axisWidgets) {
    w.icon->setPixmap(QIcon(w.iconPath).pixmap(20, 20));
    // The raw slider value is converted into a sensitivity multiplier
	// prior to being desplayed by the label.
    w.valueLabel->setText(spaceMouseSensitivityLabel(
        spaceMouseSliderToSensitivity(w.slider->value())));
    connect(w.slider, &QSlider::valueChanged, w.valueLabel,
            [w](int value) {
              w.valueLabel->setText(spaceMouseSensitivityLabel(
                  spaceMouseSliderToSensitivity(value)));
            });
    w.slider->installEventFilter(this);
    mSliders.append(w.slider);
  }
#else
  mUi->stkSpaceMouse->setCurrentWidget(mUi->pageSpaceMouseUnavailable);
#endif

  load();
}

SpaceMouseSettingsWidget::~SpaceMouseSettingsWidget() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SpaceMouseSettingsWidget::load() noexcept {
#ifdef LIBREPCB_SPACEMOUSE_AVAILABLE
  using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;
  auto load = [](QSlider* slider, QLabel* valueLabel, QCheckBox* invert,
                  const WorkspaceSettingsItem_SpaceMouse::AxisSettings& s) {
    const QSignalBlocker sliderBlocker(slider);
    const int sliderValue = spaceMouseSensitivityToSlider(s.sensitivity);
    slider->setValue(sliderValue);
    valueLabel->setText(spaceMouseSensitivityLabel(
        spaceMouseSliderToSensitivity(sliderValue)));
    invert->setChecked(s.invert);
  };
  load(mUi->sldSpaceMousePanH, mUi->lblSpaceMousePanHValue,
       mUi->chkSpaceMousePanHInvert, mSettings.get(Axis::TranslationX));
  load(mUi->sldSpaceMousePanV, mUi->lblSpaceMousePanVValue,
       mUi->chkSpaceMousePanVInvert, mSettings.get(Axis::TranslationY));
  load(mUi->sldSpaceMouseZoom, mUi->lblSpaceMouseZoomValue,
       mUi->chkSpaceMouseZoomInvert, mSettings.get(Axis::TranslationZ));
  load(mUi->sldSpaceMousePitch, mUi->lblSpaceMousePitchValue,
       mUi->chkSpaceMousePitchInvert, mSettings.get(Axis::RotationX));
  load(mUi->sldSpaceMouseRoll, mUi->lblSpaceMouseRollValue,
       mUi->chkSpaceMouseRollInvert, mSettings.get(Axis::RotationY));
  load(mUi->sldSpaceMouseYaw, mUi->lblSpaceMouseYawValue,
       mUi->chkSpaceMouseYawInvert, mSettings.get(Axis::RotationZ));
  mUi->chkSpaceMouseEnableLed->setChecked(mSettings.getLedEnabled());
#endif
}

void SpaceMouseSettingsWidget::save() noexcept {
#ifdef LIBREPCB_SPACEMOUSE_AVAILABLE
  using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;
  auto save = [](QSlider* slider, QCheckBox* invert) {
    WorkspaceSettingsItem_SpaceMouse::AxisSettings s;
    s.sensitivity = spaceMouseSliderToSensitivity(slider->value());
    s.invert = invert->isChecked();
    return s;
  };
  WorkspaceSettingsItem_SpaceMouse::AxisSettingsMap settings;
  settings[Axis::TranslationX] =
      save(mUi->sldSpaceMousePanH, mUi->chkSpaceMousePanHInvert);
  settings[Axis::TranslationY] =
      save(mUi->sldSpaceMousePanV, mUi->chkSpaceMousePanVInvert);
  settings[Axis::TranslationZ] =
      save(mUi->sldSpaceMouseZoom, mUi->chkSpaceMouseZoomInvert);
  settings[Axis::RotationX] =
      save(mUi->sldSpaceMousePitch, mUi->chkSpaceMousePitchInvert);
  settings[Axis::RotationY] =
      save(mUi->sldSpaceMouseRoll, mUi->chkSpaceMouseRollInvert);
  settings[Axis::RotationZ] =
      save(mUi->sldSpaceMouseYaw, mUi->chkSpaceMouseYawInvert);
  mSettings.set(settings);
  mSettings.setLedEnabled(mUi->chkSpaceMouseEnableLed->isChecked());
#endif
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SpaceMouseSettingsWidget::eventFilter(QObject* watched,
                                           QEvent* event) noexcept {
  // Double-clicking a Space Mouse sensitivity slider resets it to nominal
  // (1.0x, slider position 0) - QSlider has no dedicated double-click
  // signal of its own, so we use an event filter to watch for it.
  if (event->type() == QEvent::MouseButtonDblClick) {
    if (QSlider* slider = qobject_cast<QSlider*>(watched)) {
      if (mSliders.contains(slider)) {
        slider->setValue(0);
        // Consume the event so QSlider doesn't act on it as well.
		return true;  
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
