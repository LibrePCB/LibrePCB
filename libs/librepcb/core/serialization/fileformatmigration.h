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

#ifndef LIBREPCB_CORE_FILEFORMATMIGRATION_H
#define LIBREPCB_CORE_FILEFORMATMIGRATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/version.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalDirectory;

/*******************************************************************************
 *  Class FileFormatMigration
 ******************************************************************************/

/**
 * @brief Base class for any file format migration
 */
class FileFormatMigration : public QObject {
  Q_OBJECT

public:
  // Types
  struct Message {
    enum class Severity : int { Note = 0, Warning = 1, Critical = 2 };
    Version fromVersion;
    Version toVersion;
    Severity severity;
    int affectedItems;
    QString message;

    QString getSeverityStrTr() const noexcept;
  };

  // Constructors / Destructor
  FileFormatMigration() = delete;
  explicit FileFormatMigration(const Version& fromVersion,
                               const Version& toVersion,
                               QObject* parent = nullptr) noexcept;
  FileFormatMigration(const FileFormatMigration& other) = delete;
  virtual ~FileFormatMigration() noexcept;

  // Getters
  const Version& getFromVersion() const noexcept { return mFromVersion; }
  const Version& getToVersion() const noexcept { return mToVersion; }

  // General Methods
  virtual void upgradeComponentCategory(TransactionalDirectory& dir) = 0;
  virtual void upgradePackageCategory(TransactionalDirectory& dir) = 0;
  virtual void upgradeSymbol(TransactionalDirectory& dir) = 0;
  virtual void upgradePackage(TransactionalDirectory& dir) = 0;
  virtual void upgradeComponent(TransactionalDirectory& dir) = 0;
  virtual void upgradeDevice(TransactionalDirectory& dir) = 0;
  virtual void upgradeLibrary(TransactionalDirectory& dir) = 0;
  virtual void upgradeProject(TransactionalDirectory& dir,
                              QList<Message>& messages) = 0;
  virtual void upgradeWorkspaceData(TransactionalDirectory& dir) = 0;

  // Static Methods
  static QList<std::shared_ptr<FileFormatMigration>> getMigrations(
      const Version& fileFormat);

  // Operator Overloadings
  FileFormatMigration& operator=(const FileFormatMigration& rhs) = delete;

protected:  // Methods
  Message buildMessage(Message::Severity severity, const QString& message,
                       int affectedItems = -1) const noexcept;
  void upgradeVersionFile(TransactionalDirectory& dir, const QString& fileName);

protected:  // Data
  Version mFromVersion;
  Version mToVersion;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
