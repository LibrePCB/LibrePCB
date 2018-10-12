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
#include "wsi_user.h"

#include <librepcb/common/systeminfo.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WSI_User::WSI_User(const SExpression& node) : WSI_Base() {
  if (const SExpression* child = node.tryGetChildByPath("user")) {
    mName = child->getValueOfFirstChild<QString>();
  } else {
    // Fallback to system's username if no user name defined. This should
    // actually only happen once when upgrading older workspace settings.
    mName = SystemInfo::getFullUsername();
  }

  // create widget
  mWidget.reset(new QWidget());
  QVBoxLayout* layout = new QVBoxLayout(mWidget.data());
  layout->setContentsMargins(0, 0, 0, 0);
  mNameEdit.reset(new QLineEdit(mName));
  mNameEdit->setMaxLength(100);
  mNameEdit->setPlaceholderText(tr("e.g. \"John Doe\""));
  layout->addWidget(mNameEdit.data());
  layout->addWidget(
      new QLabel(tr("This name will be used as author when creating new "
                    "projects or libraries.")));
}

WSI_User::~WSI_User() noexcept {
}

/*******************************************************************************
 * Direct Access
 ******************************************************************************/

void WSI_User::setName(const QString& name) noexcept {
  mName = name;
  mNameEdit->setText(name);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WSI_User::restoreDefault() noexcept {
  mNameEdit->setText(SystemInfo::getFullUsername());
}

void WSI_User::apply() noexcept {
  mName = mNameEdit->text();
}

void WSI_User::revert() noexcept {
  mNameEdit->setText(mName);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WSI_User::serialize(SExpression& root) const {
  root.appendChild("user", mName, true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
