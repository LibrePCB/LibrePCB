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
#include "filedialog.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

QString FileDialog::getOpenFileName(QWidget *parent, const QString &caption,
                                    const QString &dir, const QString &filter,
                                    QString *            selectedFilter,
                                    QFileDialog::Options options) {
  patchOptions(options);
  return QFileDialog::getOpenFileName(parent, caption, dir, filter,
                                      selectedFilter, options);
}

QStringList FileDialog::getOpenFileNames(QWidget *            parent,
                                         const QString &      caption,
                                         const QString &      dir,
                                         const QString &      filter,
                                         QString *            selectedFilter,
                                         QFileDialog::Options options) {
  patchOptions(options);
  return QFileDialog::getOpenFileNames(parent, caption, dir, filter,
                                       selectedFilter, options);
}

QString FileDialog::getSaveFileName(QWidget *parent, const QString &caption,
                                    const QString &dir, const QString &filter,
                                    QString *            selectedFilter,
                                    QFileDialog::Options options) {
  patchOptions(options);
  return QFileDialog::getSaveFileName(parent, caption, dir, filter,
                                      selectedFilter, options);
}

QString FileDialog::getExistingDirectory(QWidget *            parent,
                                         const QString &      caption,
                                         const QString &      dir,
                                         QFileDialog::Options options) {
  patchOptions(options);
  return QFileDialog::getExistingDirectory(parent, caption, dir, options);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileDialog::patchOptions(QFileDialog::Options &options) noexcept {
  static const bool noNativeDialogs =
      qgetenv("LIBREPCB_DISABLE_NATIVE_DIALOGS") == "1";
  if (noNativeDialogs) {
    options |= QFileDialog::DontUseNativeDialog;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
