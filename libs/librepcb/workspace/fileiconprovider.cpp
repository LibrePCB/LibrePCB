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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileiconprovider.h"

#include <librepcb/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileIconProvider::FileIconProvider() noexcept : QFileIconProvider() {
}

FileIconProvider::~FileIconProvider() noexcept {
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

QIcon FileIconProvider::icon(const QFileInfo& info) const noexcept {
  if (info.isFile()) {
    if (info.suffix() == "lpp") {
      return QIcon(":/img/app/librepcb.png");
    } else {
      return QIcon(":/img/places/file.png");
    }
  } else if (info.isDir()) {
    if (project::Project::isProjectDirectory(
            FilePath(info.absoluteFilePath()))) {
      return QIcon(":/img/places/project_folder.png");
    } else if (info.isDir()) {
      return QIcon(":/img/places/folder.png");
    }
  }

  return QFileIconProvider::icon(info);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
