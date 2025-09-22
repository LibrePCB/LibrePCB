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

#ifndef LIBREPCB_EDITOR_LENGTHEDITCONTEXT_H
#define LIBREPCB_EDITOR_LENGTHEDITCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/types/length.h>
#include <librepcb/core/types/lengthunit.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class WorkspaceSettings;

namespace editor {

/*******************************************************************************
 *  Class LengthEditContext
 ******************************************************************************/

/**
 * @brief Backend configuration for the LengthEdit UI element
 */
class LengthEditContext final : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Up/down step values
   *
   * The step with lowest priority (typically the smallest value) must be the
   * first element in the list, the step with highest priority (typically the
   * largest value) the last one.
   *
   * Example:
   * {0.1mm, 1.0mm} leads to the steps 0.0mm, 0.1mm, .. 0.9mm, 1.0mm, 2.0mm, ...
   */
  struct Steps {
    static QVector<PositiveLength> generic() noexcept {
      return {
          PositiveLength(10000),  // 0.01mm
          PositiveLength(25400),  // 0.0254mm
          PositiveLength(100000),  // 0.1mm
          PositiveLength(254000),  // 0.254mm
          PositiveLength(1000000),  // 1mm
          PositiveLength(2540000),  // 2.54mm
      };
    }

    static QVector<PositiveLength> textHeight() noexcept {
      return {
          PositiveLength(100000),  // 0.1mm
          PositiveLength(254000),  // 0.254mm
          PositiveLength(500000),  // 0.5mm (default)
      };
    }

    static QVector<PositiveLength> pinLength() noexcept {
      return {
          PositiveLength(2500000),  // 2.5mm (for metric symbols)
          PositiveLength(2540000),  // 2.54mm (default)
      };
    }

    static QVector<PositiveLength> drillDiameter() noexcept {
      return {
          PositiveLength(254000),  // 0.254mm (for imperial drills)
          PositiveLength(100000),  // 0.1mm (default, for metric drills)
      };
    }
  };

  enum class StepBehavior {
    PredefinedSteps,
    HalfAndDouble,
  };

  // Constructors / Destructor
  LengthEditContext() = delete;
  LengthEditContext(const LengthEditContext& other) = delete;
  explicit LengthEditContext(const WorkspaceSettings& ws,
                             QObject* parent = nullptr) noexcept;
  virtual ~LengthEditContext() noexcept;

  // General Methods
  ui::LengthEditData getUiData() const noexcept;
  const Length& getValue() const noexcept { return mValue; }
  void setUiData(const ui::LengthEditData& data) noexcept;
  void configure(const Length& value, const QVector<PositiveLength>& steps,
                 const QString& uniqueIdentifier) noexcept;
  void configure(const UnsignedLength& value,
                 const QVector<PositiveLength>& steps,
                 const QString& uniqueIdentifier) noexcept;
  void configure(const PositiveLength& value,
                 const QVector<PositiveLength>& steps,
                 const QString& uniqueIdentifier) noexcept;
  void setValue(const Length& value) noexcept;
  void setValueUnsigned(const UnsignedLength& value) noexcept;
  void setValuePositive(const PositiveLength& value) noexcept;
  void setUnit(const LengthUnit& unit) noexcept;
  void setStepBehavior(StepBehavior behavior) noexcept;
  void stepBy(int steps) noexcept;

  // Operator Overloadings
  LengthEditContext& operator=(const LengthEditContext& rhs) = delete;

signals:
  void uiDataChanged();
  void valueChanged(const Length& value);
  void valueChangedUnsigned(const UnsignedLength& value);
  void valueChangedPositive(const PositiveLength& value);

protected:  // Methods
  LengthEditContext(const WorkspaceSettings& ws, const Length& min,
                    const Length& value, QObject* parent = nullptr) noexcept;
  void configureSettings(const QString& uniqueIdentifier) noexcept;
  void updateSingleStep() noexcept;
  void updateSingleStepPredefined() noexcept;
  void updateSingleStepHalfDouble() noexcept;
  void setValueImpl(const Length& value, bool emitValueChanged) noexcept;

protected:  // Data
  const WorkspaceSettings& mSettings;
  Length mMinimum;
  StepBehavior mStepBehavior;
  QVector<PositiveLength> mSteps;
  LengthUnit mUnit;
  Length mValue;
  Length mSingleStepUp;  ///< Zero means "no step available"
  Length mSingleStepDown;  ///< Zero means "no step available"
  QString mSettingsKey;  ///< Empty means "do not save"
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
