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
#include "lengtheditcontext.h"

#include "uihelpers.h"

#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LengthEditContext::LengthEditContext(const WorkspaceSettings& ws,
                                     const Length& min, const Length& value,
                                     QObject* parent) noexcept
  : QObject(parent),
    mSettings(ws),
    mMinimum(min),
    mStepBehavior(StepBehavior::PredefinedSteps),
    mSteps(Steps::generic()),
    mUnit(LengthUnit::millimeters()),
    mValue(value),
    mSingleStepUp(0),
    mSingleStepDown(0),
    mSettingsKey() {
  updateSingleStep();
}

LengthEditContext::LengthEditContext(const WorkspaceSettings& ws,
                                     QObject* parent) noexcept
  : LengthEditContext(ws, Length::min(), Length(0), parent) {
}

LengthEditContext::~LengthEditContext() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::LengthEditData LengthEditContext::getUiData() const noexcept {
  return ui::LengthEditData{
      l2s(mValue),  // Value
      l2s(mUnit),  // Unit
      l2s(mMinimum),  // Minimum
      (mSingleStepUp > 0),  // Can increase
      (mSingleStepDown > 0) && (mValue > mMinimum),  // Can decrease
      false,  // Increase
      false,  // Decrease
  };
}

void LengthEditContext::setUiData(const ui::LengthEditData& data) noexcept {
  setValueImpl(s2l(data.value), true);
  setUnit(s2l(data.unit));

  if (data.increase && (mSingleStepUp > 0)) {
    setValueImpl(mValue + mSingleStepUp, true);
  } else if (data.decrease && (mSingleStepDown > 0)) {
    setValueImpl(mValue - mSingleStepDown, true);
  }
}

void LengthEditContext::configure(const Length& value,
                                  const QVector<PositiveLength>& steps,
                                  const QString& uniqueIdentifier) noexcept {
  mMinimum = Length::min();
  configureSettings(uniqueIdentifier);
  setValue(value);
  mSteps = steps;
  updateSingleStep();
}

void LengthEditContext::configure(const UnsignedLength& value,
                                  const QVector<PositiveLength>& steps,
                                  const QString& uniqueIdentifier) noexcept {
  configure(*value, steps, uniqueIdentifier);
  mMinimum = Length(0);
}

void LengthEditContext::configure(const PositiveLength& value,
                                  const QVector<PositiveLength>& steps,
                                  const QString& uniqueIdentifier) noexcept {
  configure(*value, steps, uniqueIdentifier);
  mMinimum = Length(1);
}

void LengthEditContext::setValue(const Length& value) noexcept {
  setValueImpl(value, false);
}

void LengthEditContext::setValueUnsigned(const UnsignedLength& value) noexcept {
  setValueImpl(*value, false);
}

void LengthEditContext::setValuePositive(const PositiveLength& value) noexcept {
  setValueImpl(*value, false);
}

void LengthEditContext::setUnit(const LengthUnit& unit) noexcept {
  if (unit == mUnit) return;

  mUnit = unit;

  if (!mSettingsKey.isEmpty()) {
    QSettings cs;
    if (mUnit != mSettings.defaultLengthUnit.get()) {
      cs.setValue(mSettingsKey, mUnit.toStr());
    } else {
      cs.remove(mSettingsKey);
    }
  }

  emit uiDataChanged();
}

void LengthEditContext::setStepBehavior(StepBehavior behavior) noexcept {
  mStepBehavior = behavior;
  updateSingleStep();
}

void LengthEditContext::stepBy(int steps) noexcept {
  if ((mSingleStepUp > 0) && (steps > 0)) {
    setValueImpl(mValue + mSingleStepUp * steps, true);
  } else if ((mSingleStepDown > 0) && (steps < 0)) {
    setValueImpl(mValue + mSingleStepDown * steps, true);
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void LengthEditContext::configureSettings(
    const QString& uniqueIdentifier) noexcept {
  if (mUnit != mSettings.defaultLengthUnit.get()) {
    mUnit = mSettings.defaultLengthUnit.get();
    emit uiDataChanged();
  }

  mSettingsKey = uniqueIdentifier;
  if (!mSettingsKey.isEmpty()) {
    mSettingsKey.append("/unit");

    try {
      QSettings cs;
      const QString unitStr = cs.value(mSettingsKey).toString();
      if (!unitStr.isEmpty()) {
        setUnit(LengthUnit::fromString(unitStr));  // can throw
      }
    } catch (const Exception& e) {
      qWarning() << "Failed to restore length edit unit from user settings:"
                 << e.getMsg();
    }
  }
}

void LengthEditContext::updateSingleStep() noexcept {
  switch (mStepBehavior) {
    case StepBehavior::PredefinedSteps: {
      updateSingleStepPredefined();
      break;
    }
    case StepBehavior::HalfAndDouble: {
      updateSingleStepHalfDouble();
      break;
    }
    default:
      qCritical()
          << "Unhandled switch-case in LengthEditContext::updateSingleStep():"
          << static_cast<int>(mStepBehavior);
      break;
  }
}

void LengthEditContext::updateSingleStepPredefined() noexcept {
  if ((mValue == 0) || (mValue == mMinimum)) {
    return;  // keep last step values
  }

  Length up;
  Length down;
  foreach (const PositiveLength& step, mSteps) {
    if ((mValue % (*step)) == 0) {
      up = *step;
      if ((mValue.abs() > (*step)) || (down == 0)) {
        down = *step;
      }
    }
  }
  if (mValue < 0) {
    std::swap(up, down);
  }
  // Do not allow to step down if it would lead in a value smaller than the
  // minimum. This is needed for PositiveLengthEdit to avoid e.g. the next lower
  // value of 0.1mm would be 0.000001mm because it gets clipped to the minimum.
  if ((down > 0) && (mValue < (mMinimum + down))) {
    down = 0;
  }

  mSingleStepUp = up;
  mSingleStepDown = down;
}

void LengthEditContext::updateSingleStepHalfDouble() noexcept {
  if ((mValue % 2) == 0) {
    mSingleStepDown = mValue.abs() / 2;
  } else {
    mSingleStepDown = 0;
  }

  mSingleStepUp = mValue;
}

void LengthEditContext::setValueImpl(const Length& value,
                                     bool emitValueChanged) noexcept {
  if ((value != mValue) && (value >= mMinimum)) {
    mValue = value;
    updateSingleStep();
    if (emitValueChanged) {
      emit valueChanged(mValue);
      if (mValue >= 0) emit valueChangedUnsigned(UnsignedLength(mValue));
      if (mValue > 0) emit valueChangedPositive(PositiveLength(mValue));
    }
    emit uiDataChanged();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
