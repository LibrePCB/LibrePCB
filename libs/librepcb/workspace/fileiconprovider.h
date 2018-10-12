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

#ifndef LIBREPCB_WORKSPACE_LIBREPCBFILEICONPROVIDER_H
#define LIBREPCB_WORKSPACE_LIBREPCBFILEICONPROVIDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Class FileIconProvider
 ******************************************************************************/

/**
 * @brief The FileIconProvider class
 *
 * @author ubruhin
 * @date 2017-10-22
 */
class FileIconProvider final : public QFileIconProvider {
public:
  FileIconProvider() noexcept;
  ~FileIconProvider() noexcept;

  // Inherited Methods
  virtual QIcon icon(const QFileInfo& info) const noexcept override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif  // LIBREPCB_WORKSPACE_LIBREPCBFILEICONPROVIDER_H
