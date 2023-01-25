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
#include "fileformatmigration.h"

#include "../exceptions.h"
#include "../fileio/transactionaldirectory.h"
#include "../fileio/versionfile.h"
#include "../types/version.h"
#include "fileformatmigrationv01.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class FileFormatMigration::Message
 ******************************************************************************/

QString FileFormatMigration::Message::getSeverityStrTr() const noexcept {
  switch (severity) {
    case Severity::Note:
      return tr("NOTE");
    case Severity::Warning:
      return tr("WARNING");
    case Severity::Critical:
      return tr("CRITICAL");
    default:
      return "UNKNOWN";
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileFormatMigration::FileFormatMigration(const Version& fromVersion,
                                         const Version& toVersion,
                                         QObject* parent) noexcept
  : QObject(parent), mFromVersion(fromVersion), mToVersion(toVersion) {
}

FileFormatMigration::~FileFormatMigration() noexcept {
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QList<std::shared_ptr<FileFormatMigration>> FileFormatMigration::getMigrations(
    const Version& fileFormat) {
  QList<std::shared_ptr<FileFormatMigration>> migrations;
  if (fileFormat <= Version::fromString("0.1")) {
    migrations.append(std::make_shared<FileFormatMigrationV01>());
  }
  return migrations;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

FileFormatMigration::Message FileFormatMigration::buildMessage(
    Message::Severity severity, const QString& message, int affectedItems) const
    noexcept {
  const Message msg{mFromVersion, mToVersion, severity, affectedItems, message};
  const QString multiplier =
      affectedItems > 0 ? QString(" (%1x)").arg(affectedItems) : "";
  qInfo().nospace().noquote()
      << "UPGRADE " << msg.getSeverityStrTr() << multiplier << ": " << message;
  return msg;
}

void FileFormatMigration::upgradeVersionFile(TransactionalDirectory& dir,
                                             const QString& fileName) {
  const VersionFile current = VersionFile::fromByteArray(dir.read(fileName));
  if (current.getVersion() != mFromVersion) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("Unexpected file format version:\n"
                "Expected v%1, found v%2.\n"
                "File: '%3'")
            .arg(mFromVersion.toStr(), current.getVersion().toStr(),
                 dir.getAbsPath(fileName).toNative()));
  }
  dir.write(fileName, VersionFile(mToVersion).toByteArray());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
