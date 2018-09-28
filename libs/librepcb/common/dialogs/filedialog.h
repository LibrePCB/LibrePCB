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

#ifndef LIBREPCB_FILEDIALOG_H
#define LIBREPCB_FILEDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class FileDialog
 ******************************************************************************/

/**
 * @brief Wrapper around QFileDialog to slightly change its behaviour
 *
 * Using these methods ensures that no native file dialogs are used if the
 * environment variable `LIBREPCB_DISABLE_NATIVE_DIALOGS` is set to "1". This is
 * needed for automatic functional testing, as native dialogs are hard to test.
 */
class FileDialog final {
public:
  // Constructors / Destructor
  FileDialog()                        = delete;
  FileDialog(const FileDialog &other) = delete;
  ~FileDialog()                       = delete;

  static QString getOpenFileName(QWidget *            parent  = 0,
                                 const QString &      caption = QString(),
                                 const QString &      dir     = QString(),
                                 const QString &      filter  = QString(),
                                 QString *            selectedFilter = 0,
                                 QFileDialog::Options options        = 0);

  static QStringList getOpenFileNames(QWidget *            parent  = 0,
                                      const QString &      caption = QString(),
                                      const QString &      dir     = QString(),
                                      const QString &      filter  = QString(),
                                      QString *            selectedFilter = 0,
                                      QFileDialog::Options options        = 0);

  static QString getSaveFileName(QWidget *            parent  = 0,
                                 const QString &      caption = QString(),
                                 const QString &      dir     = QString(),
                                 const QString &      filter  = QString(),
                                 QString *            selectedFilter = 0,
                                 QFileDialog::Options options        = 0);

  static QString getExistingDirectory(
      QWidget *parent = 0, const QString &caption = QString(),
      const QString &      dir     = QString(),
      QFileDialog::Options options = QFileDialog::ShowDirsOnly);

private:
  static void patchOptions(QFileDialog::Options &options) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_FILEDIALOG_H
