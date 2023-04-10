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

#ifndef LIBREPCB_EDITOR_DESKTOPINTEGRATION_H
#define LIBREPCB_EDITOR_DESKTOPINTEGRATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class DesktopIntegration
 ******************************************************************************/

/**
 * @brief Allow installing/uninstalling LibrePCB to the desktop environment
 */
class DesktopIntegration final {
  Q_DECLARE_TR_FUNCTIONS(DesktopIntegration)

public:
  // Types
  enum class Status {
    NothingInstalled,
    InstalledThis,
    InstalledOther,
    InstalledUnknown,
  };
  enum class Mode { Install, Uninstall };

  // Constructors / Destructor
  DesktopIntegration() = delete;

  /**
   * @brief Check if this feature is available
   *
   * @return True if integration is available, false if not.
   */
  static bool isSupported() noexcept;

  /**
   * @brief Check which application is currently installed
   *
   * @return The current desktop integration status.
   */
  static Status getStatus() noexcept;

  /**
   * @brief Get the path to the executable to be installed
   *
   * @return FIle path to executable
   */
  static FilePath getExecutable() noexcept;

  /**
   * @brief Install this application to the desktop
   *
   * @throw  Exception    In case of an error.
   */
  static void install();

  /**
   * @brief Uninstall this application from the desktop
   *
   * @throw  Exception    In case of an error.
   */
  static void uninstall();

  /**
   * @brief Execute dialog to install or uninstall the desktop integration
   *
   * @param mode      Whether the dialog should install or uninstall.
   * @param parent    Parent widget.
   *
   * @return True on success, false otherwise.
   */
  static bool execDialog(Mode mode, QWidget* parent) noexcept;

private:
  static QHash<FilePath, QByteArray> getFileContentToInstall();
  static QSet<FilePath> loadInstalledFiles();
  static void storeInstalledFiles(const QSet<FilePath>& files);
  static void updateDatabase();
  static FilePath getDesktopFile() noexcept;
  static FilePath getConfigFile() noexcept;
  static FilePath getShareDirectory() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
