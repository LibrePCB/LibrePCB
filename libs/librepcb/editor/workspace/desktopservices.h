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

#ifndef LIBREPCB_EDITOR_DESKTOPSERVICES_H
#define LIBREPCB_EDITOR_DESKTOPSERVICES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class WorkspaceSettings;

namespace editor {

/*******************************************************************************
 *  Class DesktopServices
 ******************************************************************************/

/**
 * @brief Provides methods to access common desktop services
 *
 * Similar to `QDesktopServices`, but respecting the workspace settings (e.g.
 * custom PDF viewer).
 *
 * @see https://doc.qt.io/qt-5/qdesktopservices.html
 */
class DesktopServices final {
  Q_DECLARE_TR_FUNCTIONS(DesktopServices)

public:
  // Constructors / Destructor
  DesktopServices() = delete;
  DesktopServices(const DesktopServices& other) = delete;
  explicit DesktopServices(const WorkspaceSettings& settings, bool forceOpen,
                           QWidget* parent = nullptr) noexcept;
  ~DesktopServices() noexcept;

  // General Methods
  bool openFile(const FilePath& filePath) const noexcept;
  bool openPdf(const FilePath& filePath) const noexcept;

  // Operator Overloadings
  DesktopServices& operator=(const DesktopServices& rhs) = delete;

private:  // Methods
  bool openUrl(const QUrl& url) const noexcept;

private:  // Data
  const WorkspaceSettings& mSettings;
  const bool mForceOpen;
  QPointer<QWidget> mParent;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
