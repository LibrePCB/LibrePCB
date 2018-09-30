/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
  : QStatusBar(parent), mFields(0) {
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
  setAbsoluteCursorPosition(Point());
  setProgressBarVisible(false);
  setProgressBarPercent(0);
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

void StatusBar::setAbsoluteCursorPosition(const Point& pos) noexcept {
  mAbsPosXLabel->setText(QString("X:%1mm").arg(pos.getX().toMm(), 12, 'f', 6));
  mAbsPosYLabel->setText(QString("Y:%1mm").arg(pos.getY().toMm(), 12, 'f', 6));
}

void StatusBar::setProgressBarVisible(bool visible) noexcept {
  mProgressBar->setVisible(visible);
  mProgressBarPlaceHolder->setVisible(!visible);
}

void StatusBar::setProgressBarTextFormat(const QString& format) noexcept {
  mProgressBar->setFormat(format);
}

void StatusBar::setProgressBarPercent(int percent) noexcept {
  mProgressBar->setValue(percent);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
