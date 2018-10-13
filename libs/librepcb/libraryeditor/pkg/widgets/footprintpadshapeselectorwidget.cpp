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
#include "footprintpadshapeselectorwidget.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintPadShapeSelectorWidget::FootprintPadShapeSelectorWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mBtnRound(new QToolButton(this)),
    mBtnRect(new QToolButton(this)),
    mBtnOctagon(new QToolButton(this)) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(mBtnRound);
  layout->addWidget(mBtnRect);
  layout->addWidget(mBtnOctagon);

  mBtnRound->setIcon(QIcon(":/img/command_toolbars/shape_round.png"));
  mBtnRect->setIcon(QIcon(":/img/command_toolbars/shape_rect.png"));
  mBtnOctagon->setIcon(QIcon(":/img/command_toolbars/shape_octagon.png"));
  mBtnRound->setToolTip(tr("Round"));
  mBtnRect->setToolTip(tr("Rectangular"));
  mBtnOctagon->setToolTip(tr("Octagon"));
  mBtnRound->setCheckable(true);
  mBtnRect->setCheckable(true);
  mBtnOctagon->setCheckable(true);
  mBtnRound->setFixedWidth(32);
  mBtnRect->setFixedWidth(32);
  mBtnOctagon->setFixedWidth(32);

  connect(mBtnRound, &QToolButton::clicked, this,
          &FootprintPadShapeSelectorWidget::btnRoundToggled);
  connect(mBtnRect, &QToolButton::clicked, this,
          &FootprintPadShapeSelectorWidget::btnRectToggled);
  connect(mBtnOctagon, &QToolButton::clicked, this,
          &FootprintPadShapeSelectorWidget::btnOctagonToggled);
}

FootprintPadShapeSelectorWidget::~FootprintPadShapeSelectorWidget() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

FootprintPad::Shape FootprintPadShapeSelectorWidget::getCurrentShape() const
    noexcept {
  if (mBtnRound->isChecked()) return FootprintPad::Shape::ROUND;
  if (mBtnRect->isChecked()) return FootprintPad::Shape::RECT;
  if (mBtnOctagon->isChecked()) return FootprintPad::Shape::OCTAGON;
  return FootprintPad::Shape::ROUND;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void FootprintPadShapeSelectorWidget::setCurrentShape(
    FootprintPad::Shape shape) noexcept {
  mBtnRound->setChecked(shape == FootprintPad::Shape::ROUND);
  mBtnRect->setChecked(shape == FootprintPad::Shape::RECT);
  mBtnOctagon->setChecked(shape == FootprintPad::Shape::OCTAGON);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintPadShapeSelectorWidget::btnRoundToggled(bool checked) noexcept {
  mBtnRect->setChecked(!checked);
  mBtnOctagon->setChecked(!checked);
  emit currentShapeChanged(getCurrentShape());
}

void FootprintPadShapeSelectorWidget::btnRectToggled(bool checked) noexcept {
  mBtnRound->setChecked(!checked);
  mBtnOctagon->setChecked(!checked);
  emit currentShapeChanged(getCurrentShape());
}

void FootprintPadShapeSelectorWidget::btnOctagonToggled(bool checked) noexcept {
  mBtnRound->setChecked(!checked);
  mBtnRect->setChecked(!checked);
  emit currentShapeChanged(getCurrentShape());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
