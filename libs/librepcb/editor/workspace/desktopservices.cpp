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
#include "desktopservices.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

DesktopServices::DesktopServices(const WorkspaceSettings& settings,
                                 bool forceOpen, QWidget* parent) noexcept
  : mSettings(settings), mForceOpen(forceOpen), mParent(parent) {
}

DesktopServices::~DesktopServices() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool DesktopServices::openFile(const FilePath& filePath) const noexcept {
  const QString ext = filePath.getSuffix().toLower();
  if (ext == "pdf") {
    return openPdf(filePath);
  } else {
    return openUrl(QUrl::fromLocalFile(filePath.toNative()));
  }
}

bool DesktopServices::openPdf(const FilePath& filePath) const noexcept {
  // Logic temporarily removed since workspace settings will be added later.
  return openUrl(QUrl::fromLocalFile(filePath.toNative()));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool DesktopServices::openUrl(const QUrl& url) const noexcept {
  if (QDesktopServices::openUrl(url)) {
    return true;
  } else {
    qCritical() << "Failed to open URL with QDesktopServices:" << url;
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
