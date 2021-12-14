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
#include "directorylockhandlerdialog.h"

#include "ui_directorylockhandlerdialog.h"

#include <librepcb/core/exceptions.h>

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

DirectoryLockHandlerDialog::DirectoryLockHandlerDialog(
    const FilePath& directory, const QString& user, bool allowOverrideLock,
    QWidget* parent) noexcept
  : QDialog(parent), mUi(new Ui::DirectoryLockHandlerDialog) {
  mUi->setupUi(this);
  mUi->lblDescription->setText(
      mUi->lblDescription->text().arg(directory.toNative(), user));
  mUi->lblDisclaimer->setVisible(allowOverrideLock);

  if (allowOverrideLock) {
    // Add "accept risk" checkbox to buttonbox.
    QCheckBox* cbxAcceptRisk = new QCheckBox(tr("I accept the risk."), this);
    mUi->buttonBox->addButton(cbxAcceptRisk, QDialogButtonBox::ActionRole);

    // Add "override lock" button to buttonbox.
    QPushButton* btnOverride = mUi->buttonBox->addButton(
        tr("Open anyway"), QDialogButtonBox::DestructiveRole);
    btnOverride->setEnabled(false);
    connect(btnOverride, &QPushButton::clicked, this, &QDialog::accept);
    connect(cbxAcceptRisk, &QCheckBox::toggled, btnOverride,
            &QPushButton::setEnabled);
  }

  adjustSize();
}

DirectoryLockHandlerDialog::~DirectoryLockHandlerDialog() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

DirectoryLock::LockHandlerCallback
    DirectoryLockHandlerDialog::createDirectoryLockCallback(
        QWidget* parent) noexcept {
  return [parent](const FilePath& p, DirectoryLock::LockStatus status,
                  const QString& user) {
    bool allowOverride =
        (status == DirectoryLock::LockStatus::LockedByOtherUser) ||
        (status == DirectoryLock::LockStatus::LockedByUnknownApp);
    DirectoryLockHandlerDialog dialog(p, user, allowOverride, parent);
    if (dialog.exec() == QDialog::Accepted) {
      return true;
    }
    throw UserCanceled(__FILE__, __LINE__);
  };
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
