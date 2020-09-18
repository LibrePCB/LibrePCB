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

#ifndef LIBREPCB_STATUSBAR_H
#define LIBREPCB_STATUSBAR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../units/lengthunit.h"
#include "../units/point.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class StatusBar
 ******************************************************************************/

/**
 * @brief The StatusBar class extends QStatusBar for some commonly used fields
 */
class StatusBar final : public QStatusBar {
  Q_OBJECT

public:
  // Types
  enum Field {
    AbsolutePosition = 1 << 0,
    ProgressBar = 1 << 1,
  };
  Q_DECLARE_FLAGS(Fields, Field);

  // Constructors / Destructor
  explicit StatusBar(QWidget* parent = nullptr) noexcept;
  StatusBar(const StatusBar& other) = delete;
  ~StatusBar() noexcept;

  // Getters
  Fields getFields() const noexcept { return mFields; }

  // Setters
  void setFields(Fields fields) noexcept;
  void setField(Field field, bool enable) noexcept;
  void setLengthUnit(const LengthUnit& unit) noexcept;
  void setAbsoluteCursorPosition(const Point& pos) noexcept;
  void setProgressBarTextFormat(const QString& format) noexcept;
  void setProgressBarPercent(int percent) noexcept;

  // Operator Overloadings
  StatusBar& operator=(const StatusBar& rhs) = delete;

private:  // Methods
  void updateAbsoluteCursorPosition() noexcept;

private:  // Data
  Fields mFields;
  LengthUnit mLengthUnit;
  Point mAbsoluteCursorPosition;
  QScopedPointer<QLabel> mAbsPosXLabel;
  QScopedPointer<QLabel> mAbsPosYLabel;
  QScopedPointer<QProgressBar> mProgressBar;
  QScopedPointer<QWidget> mProgressBarPlaceHolder;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::StatusBar::Fields)

#endif  // LIBREPCB_STATUSBAR_H
