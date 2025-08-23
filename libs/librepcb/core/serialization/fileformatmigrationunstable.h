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

#ifndef LIBREPCB_CORE_FILEFORMATMIGRATIONUNSTABLE_H
#define LIBREPCB_CORE_FILEFORMATMIGRATIONUNSTABLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileformatmigrationv1.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/*******************************************************************************
 *  Class FileFormatMigrationUnstable
 ******************************************************************************/

/**
 * @brief Migration to upgrade a previous unstable file format
 *
 * This class overrides the stable file format migration class to perform
 * only a partial upgrade. This allows to upgrade file from the previous
 * unstable file format (master branch) to the latest unstable file format
 * (feature branch). This upgrade is only performed when the environment
 * variable `LIBREPCB_UPGRADE_UNSTABLE=1` is set.
 */
class FileFormatMigrationUnstable final : public FileFormatMigrationV1 {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit FileFormatMigrationUnstable(QObject* parent = nullptr) noexcept;
  FileFormatMigrationUnstable(const FileFormatMigrationUnstable& other) =
      delete;
  ~FileFormatMigrationUnstable() noexcept;

  // General Methods
  virtual void upgradeComponentCategory(TransactionalDirectory& dir) override;
  virtual void upgradePackageCategory(TransactionalDirectory& dir) override;
  virtual void upgradeSymbol(TransactionalDirectory& dir) override;
  virtual void upgradePackage(TransactionalDirectory& dir) override;
  virtual void upgradeComponent(TransactionalDirectory& dir) override;
  virtual void upgradeDevice(TransactionalDirectory& dir) override;
  virtual void upgradeLibrary(TransactionalDirectory& dir) override;
  virtual void upgradeWorkspaceData(TransactionalDirectory& dir) override;

  // Operator Overloadings
  FileFormatMigrationUnstable& operator=(
      const FileFormatMigrationUnstable& rhs) = delete;

protected:
  virtual void upgradeOutputJobs(SExpression& root, ProjectContext& context) override;
  virtual void upgradeBoard(SExpression& root) override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
