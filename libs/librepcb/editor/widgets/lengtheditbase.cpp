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
#include "lengtheditbase.h"

#include "../editorcommandset.h"

#include <librepcb/core/utils/mathparser.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LengthEditBase::LengthEditBase(const Length& min, const Length& max,
                               const Length& value, QWidget* parent) noexcept
  : QAbstractSpinBox(parent),
    mChangeUnitAction(nullptr),
    mDefaultUnit(LengthUnit::millimeters()),
    mSelectedUnit(std::nullopt),
    mMinimum(min),
    mMaximum(max),
    mValue(value),
    mStepBehavior(StepBehavior::PredefinedSteps),
    mSteps(Steps::generic()),
    mSingleStepUp(0),
    mSingleStepDown(0),
    // Additional size for the QAction inside the QLineEdit because
    // QAbstractSpinBox does not respect it.
    mAdditionalSize(30, 0),
    mSettingsKey() {
  Q_ASSERT((mValue >= mMinimum) && (mValue <= mMaximum));

  // Add action to change unit.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  mChangeUnitAction = cmd.inputUnitChange.createAction(
      this, this, &LengthEditBase::changeUnitActionTriggered,
      EditorCommand::ActionFlag::WidgetShortcut);
  lineEdit()->addAction(mChangeUnitAction, QLineEdit::TrailingPosition);
  addAction(mChangeUnitAction);  // Required to get keyboard shortcut working.

  // Ugly hack to make sizeHint() and minimumSizeHint() working properly.
  // QAbstractSpinBox uses (among others) the special value text to calculate
  // the size hint, so let's set it to a dummy string which is long enough to
  // represent typical length values.
  setSpecialValueText("000.000 mils");

  // Setup QLineEdit.
  lineEdit()->setPlaceholderText(tr("Enter numeric expression"));
  lineEdit()->setMaxLength(50);
  updateText();

  // editingFinished from the QLineEdit is not always emitted (e.g. when
  // leaving focus), therefore we need to use editingFinished from
  // QAbstractSpinBox.
  connect(this, &LengthEditBase::editingFinished, this,
          &LengthEditBase::updateText);
  connect(lineEdit(), &QLineEdit::textEdited, this,
          &LengthEditBase::updateValueFromText);
}

LengthEditBase::~LengthEditBase() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const LengthUnit& LengthEditBase::getDisplayedUnit() const noexcept {
  return mSelectedUnit ? *mSelectedUnit : mDefaultUnit;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LengthEditBase::setDefaultValueToolTip(const Length& value) noexcept {
  setToolTip(tr("Default value:") % " " % value.toMmString() % " mm");
}

void LengthEditBase::setDefaultUnit(const LengthUnit& unit) noexcept {
  if (unit != mDefaultUnit) {
    mDefaultUnit = unit;
    updateText();
  }
}

void LengthEditBase::setChangeUnitActionVisible(bool visible) noexcept {
  mChangeUnitAction->setVisible(visible);
}

void LengthEditBase::setStepBehavior(StepBehavior behavior) noexcept {
  mStepBehavior = behavior;
  updateSingleStep();
  update();  // step buttons might need to be repainted
}

void LengthEditBase::setSteps(const QVector<PositiveLength>& steps) noexcept {
  mSteps = steps;
  updateSingleStep();
  update();  // step buttons might need to be repainted
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LengthEditBase::resetUnit() noexcept {
  if (mSelectedUnit) {
    mSelectedUnit = std::nullopt;
    updateText();
  }
}

void LengthEditBase::configureClientSettings(
    const QString& uniqueIdentifier) noexcept {
  mSettingsKey = uniqueIdentifier % "/unit";

  try {
    QSettings clientSettings;
    const QString unitStr = clientSettings.value(mSettingsKey).toString();
    const std::optional<LengthUnit> unit = (!unitStr.isEmpty())
        ? std::make_optional(LengthUnit::fromString(unitStr))  // can throw
        : std::nullopt;
    if (unit != mSelectedUnit) {
      mSelectedUnit = unit;
      updateText();
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to restore length edit unit from user settings:"
               << e.getMsg();
  }
}

void LengthEditBase::configure(const LengthUnit& defaultUnit,
                               const QVector<PositiveLength>& steps,
                               const QString& uniqueIdentifier) noexcept {
  setDefaultUnit(defaultUnit);
  setSteps(steps);
  configureClientSettings(uniqueIdentifier);
}

void LengthEditBase::stepBy(int steps) {
  if ((mSingleStepUp > 0) && (steps > 0)) {
    setValueImpl(mValue + mSingleStepUp * steps);
  } else if ((mSingleStepDown > 0) && (steps < 0)) {
    setValueImpl(mValue + mSingleStepDown * steps);
  }
}

/*******************************************************************************
 *  Reimplemented Methods
 ******************************************************************************/

QSize LengthEditBase::minimumSizeHint() const {
  return QAbstractSpinBox::minimumSizeHint() + mAdditionalSize;
}

QSize LengthEditBase::sizeHint() const {
  return QAbstractSpinBox::sizeHint() + mAdditionalSize;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

QAbstractSpinBox::StepEnabled LengthEditBase::stepEnabled() const {
  QAbstractSpinBox::StepEnabled enabled = QAbstractSpinBox::StepNone;
  if (!isReadOnly()) {
    if ((mSingleStepUp > 0) && (mValue < mMaximum)) {
      enabled |= QAbstractSpinBox::StepUpEnabled;
    }
    if ((mSingleStepDown > 0) && (mValue > mMinimum)) {
      enabled |= QAbstractSpinBox::StepDownEnabled;
    }
  }
  return enabled;
}

void LengthEditBase::setValueImpl(Length value) noexcept {
  // Always clip the value to the allowed range! Otherwise the value might not
  // be convertible into the constrained Length type of derived classes!
  value = qBound(mMinimum, value, mMaximum);

  // To avoid unnecessary clearing the QLineEdit selection, only update the
  // value (and therefore the text) if really needed.
  if (value != mValue) {
    const Length diff = value - mValue;
    mValue = value;
    updateSingleStep();
    updateText();
    valueChangedImpl(diff);
    update();  // step buttons might need to be repainted
  }
}

void LengthEditBase::updateValueFromText(QString text) noexcept {
  try {
    std::optional<LengthUnit> unit = LengthUnit::extractFromExpression(text);
    if (!unit) {
      unit = getDisplayedUnit();  // if no unit specified, use current unit
    }
    const MathParser::Result result = MathParser().parse(text);
    if (result.valid) {
      Length value = unit->convertFromUnit(result.value);  // can throw
      // Only accept values in the allowed range.
      if ((value >= mMinimum) && (value <= mMaximum)) {
        const Length diff = value - mValue;
        mValue = value;
        setSelectedUnit(*unit);
        updateSingleStep();
        // In contrast to setValueImpl(), do NOT call updateText() to avoid
        // disturbing the user while writing the text!
        valueChangedImpl(diff);
        update();  // step buttons might need to be repainted
      } else {
        qWarning() << "Entered length text was a valid number, but "
                      "outside the allowed range:"
                   << text;
      }
    }
  } catch (const Exception&) {
    qWarning() << "Entered length text was a valid expression, but "
                  "evaluated to an invalid number:"
               << text;
  }
}

void LengthEditBase::updateSingleStep() noexcept {
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
          << "Unhandled switch-case in LengthEditBase::updateSingleStep():"
          << static_cast<int>(mStepBehavior);
      break;
  }
}

void LengthEditBase::updateSingleStepPredefined() noexcept {
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

void LengthEditBase::updateSingleStepHalfDouble() noexcept {
  if ((mValue % 2) == 0) {
    mSingleStepDown = mValue.abs() / 2;
  } else {
    mSingleStepDown = 0;
  }

  mSingleStepUp = mValue;
}

void LengthEditBase::updateText() noexcept {
  lineEdit()->setText(getValueStr(getDisplayedUnit()));
}

void LengthEditBase::changeUnitActionTriggered() noexcept {
  QMenu menu(this);
  QActionGroup group(&menu);
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    QString text = getValueStr(unit);
    if (unit == LengthUnit::nanometers()) {
      text += " (" % tr("internal") % ")";
    }
    if (unit == mDefaultUnit) {
      text += " [" % tr("default") % "]";
    }
    QAction* action = menu.addAction(text);
    group.addAction(action);
    action->setCheckable(true);
    action->setChecked(unit == getDisplayedUnit());
    connect(action, &QAction::triggered, [this, unit]() {
      setSelectedUnit(unit);
      updateText();
    });
  }

  // Note: Don't use QCursor::pos() since it would be completely wrong when the
  // menu is triggered by the keyboard shortcut.
  menu.exec(mapToGlobal(QPoint(0, height())));
}

void LengthEditBase::setSelectedUnit(const LengthUnit& unit) noexcept {
  std::optional<LengthUnit> selectedUnit =
      (unit != mDefaultUnit) ? std::make_optional(unit) : std::nullopt;
  if (selectedUnit != mSelectedUnit) {
    mSelectedUnit = selectedUnit;
    saveSelectedUnit();
    emit displayedUnitChanged(getDisplayedUnit());
  }
}

void LengthEditBase::saveSelectedUnit() noexcept {
  if (!mSettingsKey.isEmpty()) {
    QSettings clientSettings;
    if (mSelectedUnit) {
      clientSettings.setValue(mSettingsKey, mSelectedUnit->toStr());
    } else {
      clientSettings.remove(mSettingsKey);
    }
  }
}

QString LengthEditBase::getValueStr(const LengthUnit& unit) const noexcept {
  if (unit == LengthUnit::nanometers()) {
    return QString::number(mValue.toNm()) % " " % unit.toShortStringTr();
  } else {
    // Show only a limited number of decimals to avoid very odd numbers with
    // many decimals due to converting between different units (e.g. a value
    // of 0.1mm displayed in mils is 3.937007874, but such a number is annoying
    // in a GUI). The underlying value is of course not truncated, so it should
    // be fine to reduce the displayed number of decimals.
    return unit.format(mValue, locale());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
