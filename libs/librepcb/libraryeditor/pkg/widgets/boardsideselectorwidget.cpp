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
#include "boardsideselectorwidget.h"

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

BoardSideSelectorWidget::BoardSideSelectorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mBtnTop(new QToolButton(this)),
    mBtnBottom(new QToolButton(this)) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(mBtnTop);
  layout->addWidget(mBtnBottom);

  mBtnTop->setIcon(QIcon(":/img/command_toolbars/pad_top.png"));
  mBtnBottom->setIcon(QIcon(":/img/command_toolbars/pad_bottom.png"));
  mBtnTop->setToolTip(tr("Top"));
  mBtnBottom->setToolTip(tr("Bottom"));
  mBtnTop->setCheckable(true);
  mBtnBottom->setCheckable(true);
  mBtnTop->setChecked(true);
  mBtnBottom->setChecked(true);
  mBtnTop->setFixedWidth(32);
  mBtnBottom->setFixedWidth(32);

  connect(mBtnTop, &QToolButton::toggled, this,
          &BoardSideSelectorWidget::btnTopToggled);
  connect(mBtnBottom, &QToolButton::toggled, this,
          &BoardSideSelectorWidget::btnBottomToggled);
}

BoardSideSelectorWidget::~BoardSideSelectorWidget() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

FootprintPad::BoardSide BoardSideSelectorWidget::getCurrentBoardSide() const
    noexcept {
  if (mBtnTop->isChecked()) return FootprintPad::BoardSide::TOP;
  if (mBtnBottom->isChecked()) return FootprintPad::BoardSide::BOTTOM;
  return FootprintPad::BoardSide::TOP;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BoardSideSelectorWidget::setCurrentBoardSide(
    FootprintPad::BoardSide side) noexcept {
  mBtnTop->setChecked(side == FootprintPad::BoardSide::TOP);
  mBtnBottom->setChecked(side == FootprintPad::BoardSide::BOTTOM);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardSideSelectorWidget::btnTopToggled(bool checked) noexcept {
  mBtnBottom->setChecked(!checked);
  emit currentBoardSideChanged(getCurrentBoardSide());
}

void BoardSideSelectorWidget::btnBottomToggled(bool checked) noexcept {
  mBtnTop->setChecked(!checked);
  emit currentBoardSideChanged(getCurrentBoardSide());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
