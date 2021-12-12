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
#include "halignactiongroup.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

HAlignActionGroup::HAlignActionGroup(QWidget* parent) noexcept
  : QActionGroup(parent), mValue(HAlign::left()) {
  QAction* left =
      addAction(QIcon(":img/command_toolbars/align_horizontal_left.png"),
                tr("Align left"));
  left->setCheckable(true);
  left->setData(QVariant::fromValue(HAlign::left()));

  QAction* center =
      addAction(QIcon(":img/command_toolbars/align_horizontal_center.png"),
                tr("Align center"));
  center->setCheckable(true);
  center->setData(QVariant::fromValue(HAlign::center()));

  QAction* right =
      addAction(QIcon(":img/command_toolbars/align_horizontal_right.png"),
                tr("Align right"));
  right->setCheckable(true);
  right->setData(QVariant::fromValue(HAlign::right()));

  updateSelection();

  connect(this, &HAlignActionGroup::triggered, this,
          &HAlignActionGroup::actionTriggered);
}

HAlignActionGroup::~HAlignActionGroup() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void HAlignActionGroup::setValue(const HAlign& value) noexcept {
  if (value != mValue) {
    mValue = value;
    updateSelection();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void HAlignActionGroup::updateSelection() noexcept {
  foreach (QAction* action, actions()) {
    if (action->data().value<HAlign>() == mValue) {
      action->setChecked(true);
      return;
    }
  }
  Q_ASSERT(false);
}

void HAlignActionGroup::actionTriggered(QAction* action) noexcept {
  Q_ASSERT(action);
  Q_ASSERT(action->data().canConvert<HAlign>());
  HAlign value = action->data().value<HAlign>();
  if (value != mValue) {
    mValue = value;
    emit valueChanged(mValue);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
