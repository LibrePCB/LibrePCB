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
#include "signalrolecombobox.h"

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

SignalRoleComboBox::SignalRoleComboBox(QWidget* parent) noexcept
  : QWidget(parent), mComboBox(new QComboBox(this)) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mComboBox);

  foreach (const SignalRole& role, SignalRole::getAllRoles()) {
    mComboBox->addItem(role.getNameTr());
  }
  mComboBox->setCurrentIndex(0);
  connect(
      mComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &SignalRoleComboBox::currentIndexChanged);
}

SignalRoleComboBox::~SignalRoleComboBox() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

SignalRole SignalRoleComboBox::getCurrentItem() const noexcept {
  int index = mComboBox->currentIndex();
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < SignalRole::getAllRoles().count());
  return SignalRole::getAllRoles().value(index);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SignalRoleComboBox::setCurrentItem(const SignalRole& role) noexcept {
  int index = SignalRole::getAllRoles().indexOf(role);
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < SignalRole::getAllRoles().count());
  mComboBox->setCurrentIndex(index);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SignalRoleComboBox::currentIndexChanged(int index) noexcept {
  Q_UNUSED(index);
  emit currentItemChanged(getCurrentItem());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
