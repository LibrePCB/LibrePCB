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
#include "movealigndialog.h"

#include "ui_movealigndialog.h"

#include <librepcb/core/utils/toolbox.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MoveAlignDialog::MoveAlignDialog(const QList<Point>& positions,
                                 const QString& settingsPrefix,
                                 QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::MoveAlignDialog),
    mSettingsPrefix(settingsPrefix),
    mPositions(positions),
    mNewPositions(positions) {
  mUi->setupUi(this);
  connect(mUi->rbtnModeAbsolute, &QRadioButton::toggled, this, [&](bool c) {
    if (c) {
      const Point ref = mPositionsOrdered.value(0);
      mUi->edtX->setValue(mUi->edtX->getValue() + ref.getX());
      mUi->edtY->setValue(mUi->edtY->getValue() + ref.getY());
    }
  });
  connect(mUi->rbtnModeRelative, &QRadioButton::toggled, this, [&](bool c) {
    if (c) {
      const Point ref = mPositionsOrdered.value(0);
      mUi->edtX->setValue(mUi->edtX->getValue() - ref.getX());
      mUi->edtY->setValue(mUi->edtY->getValue() - ref.getY());
    }
  });
  connect(mUi->btnCenterHorizontally, &QToolButton::toggled, mUi->edtX,
          &LengthEdit::setDisabled);
  connect(mUi->btnCenterVertically, &QToolButton::toggled, mUi->edtY,
          &LengthEdit::setDisabled);
  connect(mUi->cbxIntervalX, &QCheckBox::toggled, mUi->edtIntervalX,
          &LengthEdit::setEnabled);
  connect(mUi->cbxIntervalY, &QCheckBox::toggled, mUi->edtIntervalY,
          &LengthEdit::setEnabled);
  connect(mUi->btnHorizontally, &QToolButton::clicked, this, [&]() {
    mUi->edtIntervalY->setValue(Length(0));
    mUi->cbxIntervalY->setChecked(true);
  });
  connect(mUi->btnVertically, &QToolButton::clicked, this, [&]() {
    mUi->edtIntervalX->setValue(Length(0));
    mUi->cbxIntervalX->setChecked(true);
  });
  connect(mUi->buttonBox, &QDialogButtonBox::accepted, this,
          &MoveAlignDialog::accept);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &MoveAlignDialog::reject);

  // React on settings changed.
  connect(mUi->edtX, &LengthEdit::valueChanged, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->edtY, &LengthEdit::valueChanged, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->btnCenterHorizontally, &QToolButton::toggled, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->btnCenterVertically, &QToolButton::toggled, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->cbxIntervalX, &QCheckBox::toggled, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->cbxIntervalY, &QCheckBox::toggled, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->edtIntervalX, &LengthEdit::valueChanged, this,
          &MoveAlignDialog::updateNewPositions);
  connect(mUi->edtIntervalY, &LengthEdit::valueChanged, this,
          &MoveAlignDialog::updateNewPositions);

  // Calculate spread in X- and Y-direction.
  QList<Length> xVals, yVals;  // May be empty.
  Length spreadX, spreadY;
  for (const Point& p : mPositions) {
    xVals.append(p.getX());
    yVals.append(p.getY());
  }
  std::sort(xVals.begin(), xVals.end());
  std::sort(yVals.begin(), yVals.end());
  if (mPositions.count() >= 2) {
    spreadX = xVals.last() - xVals.first();
    spreadY = yVals.last() - yVals.first();
  } else {
    mUi->gbxInterval->setEnabled(false);
  }

  // Calculate order how to interpret the input positions.
  mPositionsOrdered = Toolbox::toList(Toolbox::toSet(mPositions));
  std::sort(mPositionsOrdered.begin(), mPositionsOrdered.end(),
            [&](const Point& a, const Point& b) {
              if (spreadX > spreadY) {
                // Horizontal: Sort on X first, then on Y value.
                if (a.getX() != b.getX()) {
                  return a.getX() < b.getX();
                } else {
                  return a.getY() > b.getY();
                }
              } else {
                // Vertical: Sort on Y first, then on X value.
                if (a.getY() != b.getY()) {
                  return a.getY() > b.getY();
                } else {
                  return a.getX() < b.getX();
                }
              }
            });

  // Determine reference position.
  const Point refPos = mPositionsOrdered.value(0);
  mUi->edtX->setValue(refPos.getX());
  mUi->edtY->setValue(refPos.getY());

  // Get steps in X- and Y-direction.
  QList<Length> xSteps, ySteps;  // May be empty!
  for (int i = 1; i < mPositionsOrdered.count(); ++i) {
    const Point p = mPositionsOrdered.value(i);
    const Point p0 = mPositionsOrdered.value(i - 1);
    xSteps.append(p.getX() - p0.getX());
    ySteps.append(p.getY() - p0.getY());
  }
  if (mPositionsOrdered.count() >= 2) {
    mDefaultInterval.setX(xSteps.first());
    mDefaultInterval.setY(-ySteps.first());
  }
  mUi->edtIntervalX->setValue(mDefaultInterval.getX());
  mUi->edtIntervalY->setValue(mDefaultInterval.getY());

  // Check if the interval is constant between each item.
  mUi->cbxIntervalX->setChecked(Toolbox::toSet(xSteps).count() == 1);
  mUi->cbxIntervalY->setChecked(Toolbox::toSet(ySteps).count() == 1);

  // If only one object is selected, choose relative mode by default and
  // don't support centering.
  if (mPositions.count() == 1) {
    mUi->rbtnModeRelative->setChecked(true);
    mUi->btnCenterHorizontally->setEnabled(false);
    mUi->btnCenterVertically->setEnabled(false);
  }

  // Move focus into X coordiante for to allow editing it immediately.
  mUi->edtX->setFocus();

  // Install event filter on group boxes to make the Return key working.
  mUi->gbxRefPos->installEventFilter(this);
  mUi->gbxInterval->installEventFilter(this);

  // Load client settings.
  QSettings cs;
  QSize windowSize = cs.value(mSettingsPrefix % "/window_size").toSize();
  if (!windowSize.isEmpty()) {
    resize(windowSize);
  }
}

MoveAlignDialog::~MoveAlignDialog() noexcept {
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/window_size", size());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool MoveAlignDialog::eventFilter(QObject* watched, QEvent* event) noexcept {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* e = static_cast<QKeyEvent*>(event);
    if (e->key() == Qt::Key_Return) {
      accept();
      return true;
    }
  }
  return QDialog::eventFilter(watched, event);
}

void MoveAlignDialog::updateNewPositions() noexcept {
  // Calculate movement for all positions.
  const Point firstPosOld = mPositionsOrdered.value(0);
  Point firstPosNew(mUi->edtX->getValue(), mUi->edtY->getValue());
  if (mUi->rbtnModeRelative->isChecked()) {
    firstPosNew += firstPosOld;
  }
  const Point deltaPos = firstPosNew - firstPosOld;

  // Calculate new positions.
  QList<Point> positions = mPositions;
  for (Point& pos : positions) {
    Q_ASSERT(mPositionsOrdered.contains(pos));
    const int index = mPositionsOrdered.indexOf(pos);
    pos += deltaPos;
    if (mUi->cbxIntervalX->isChecked()) {
      pos.setX(firstPosNew.getX() + mUi->edtIntervalX->getValue() * index);
    }
    if (mUi->cbxIntervalY->isChecked()) {
      pos.setY(firstPosNew.getY() - mUi->edtIntervalY->getValue() * index);
    }
  }

  // Apply centering.
  if (!positions.isEmpty()) {
    Point offset = calcCenter(positions);
    if (!mUi->btnCenterHorizontally->isChecked()) {
      offset.setX(0);
    }
    if (!mUi->btnCenterVertically->isChecked()) {
      offset.setY(0);
    }
    for (Point& pos : positions) {
      pos -= offset;
    }
  }

  // Update UI.
  mUi->btnVertically->setEnabled((!mUi->cbxIntervalX->isChecked()) ||
                                 (mUi->edtIntervalX->getValue() != 0));
  mUi->btnHorizontally->setEnabled((!mUi->cbxIntervalY->isChecked()) ||
                                   (mUi->edtIntervalY->getValue() != 0));

  // Memorize and notify about changes.
  if (positions != mNewPositions) {
    mNewPositions = positions;
    emit positionsChanged(mNewPositions);
  }
}

Point MoveAlignDialog::calcCenter(const QList<Point>& p) noexcept {
  if (p.isEmpty()) {
    return Point();
  }
  auto cmpX = [&](const Point& a, const Point& b) {
    return a.getX() < b.getX();
  };
  auto cmpY = [&](const Point& a, const Point& b) {
    return a.getY() < b.getY();
  };
  const Length minX = std::min_element(p.begin(), p.end(), cmpX)->getX();
  const Length maxX = std::max_element(p.begin(), p.end(), cmpX)->getX();
  const Length minY = std::min_element(p.begin(), p.end(), cmpY)->getY();
  const Length maxY = std::max_element(p.begin(), p.end(), cmpY)->getY();
  return Point((minX + maxX) / 2, ((minY + maxY) / 2));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
