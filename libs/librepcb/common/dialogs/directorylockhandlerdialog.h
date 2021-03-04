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

#ifndef LIBREPCB_DIRECTORYLOCKHANDLERDIALOG_H
#define LIBREPCB_DIRECTORYLOCKHANDLERDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/directorylock.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace Ui {
class DirectoryLockHandlerDialog;
}

/*******************************************************************************
 *  Class DirectoryLockHandlerDialog
 ******************************************************************************/

/**
 * @brief The DirectoryLockHandlerDialog class
 */
class DirectoryLockHandlerDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  DirectoryLockHandlerDialog() = delete;
  DirectoryLockHandlerDialog(const DirectoryLockHandlerDialog& other) = delete;
  DirectoryLockHandlerDialog(const FilePath& directory, const QString& user,
                             bool allowOverrideLock,
                             QWidget* parent = nullptr) noexcept;
  ~DirectoryLockHandlerDialog() noexcept;

  // Operator Overloadings
  DirectoryLockHandlerDialog& operator=(const DirectoryLockHandlerDialog& rhs) =
      delete;

  static DirectoryLock::LockHandlerCallback createDirectoryLockCallback(
      QWidget* parent = nullptr) noexcept;

private:  // Data
  QScopedPointer<Ui::DirectoryLockHandlerDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
