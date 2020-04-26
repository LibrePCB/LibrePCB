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
#include "statusbar.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StatusBar::StatusBar(QWidget* parent) noexcept
  : QStatusBar(parent), mFields(0), mLengthUnit(), mAbsoluteCursorPosition() {
  // absolute position x
  mAbsPosXLabel.reset(new QLabel());
  mAbsPosXLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  mAbsPosXLabel->setFont(QFont("monospace"));
  addPermanentWidget(mAbsPosXLabel.data());

  // absolute position y
  mAbsPosYLabel.reset(new QLabel());
  mAbsPosYLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  mAbsPosYLabel->setFont(QFont("monospace"));
  addPermanentWidget(mAbsPosYLabel.data());

  // progress bar
  mProgressBar.reset(new QProgressBar());
  mProgressBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  mProgressBar->setFixedWidth(200);
  mProgressBar->setMinimum(0);
  mProgressBar->setMaximum(100);
  addPermanentWidget(mProgressBar.data());

  // progress bar placeholder to reserve space
  mProgressBarPlaceHolder.reset(new QWidget());
  mProgressBarPlaceHolder->setFixedWidth(200);
  addPermanentWidget(mProgressBarPlaceHolder.data());

  // init
  setFields(0);
  updateAbsoluteCursorPosition();
  setProgressBarPercent(100);
}

StatusBar::~StatusBar() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void StatusBar::setFields(Fields fields) noexcept {
  mAbsPosXLabel->setVisible(fields & AbsolutePosition);
  mAbsPosYLabel->setVisible(fields & AbsolutePosition);
}

void StatusBar::setField(Field field, bool enable) noexcept {
  if (enable) {
    mFields |= field;
  } else {
    mFields &= ~field;
  }
  setFields(mFields);
}

void StatusBar::setLengthUnit(const LengthUnit& unit) noexcept {
  mLengthUnit = unit;
  updateAbsoluteCursorPosition();
}

void StatusBar::setAbsoluteCursorPosition(const Point& pos) noexcept {
  mAbsoluteCursorPosition = pos;
  updateAbsoluteCursorPosition();
}

void StatusBar::setProgressBarTextFormat(const QString& format) noexcept {
  mProgressBar->setFormat(format);
}

void StatusBar::setProgressBarPercent(int percent) noexcept {
  if (percent < 100) {
    mProgressBarPlaceHolder->hide();
    mProgressBar->setValue(percent);
    mProgressBar->show();
  } else {
    mProgressBar->hide();
    mProgressBarPlaceHolder->show();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void StatusBar::updateAbsoluteCursorPosition() noexcept {
  mAbsPosXLabel->setText(
      QString("X:%1%2")
          .arg(mLengthUnit.convertToUnit(mAbsoluteCursorPosition.getX()), 12,
               'f', mLengthUnit.getReasonableNumberOfDecimals())
          .arg(mLengthUnit.toShortStringTr()));
  mAbsPosYLabel->setText(
      QString("Y:%1%2")
          .arg(mLengthUnit.convertToUnit(mAbsoluteCursorPosition.getY()), 12,
               'f', mLengthUnit.getReasonableNumberOfDecimals())
          .arg(mLengthUnit.toShortStringTr()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
