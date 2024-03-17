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
#include "workspace.h"

#include "../application.h"
#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../fileio/fileutils.h"
#include "../fileio/transactionalfilesystem.h"
#include "../fileio/versionfile.h"
#include "../library/library.h"
#include "../project/project.h"
#include "../serialization/fileformatmigration.h"
#include "workspacelibrarydb.h"
#include "workspacesettings.h"

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Workspace::Workspace(const FilePath& wsPath, const QString& dataDirName,
                     DirectoryLock::LockHandlerCallback lockCallback)
  : QObject(nullptr),
    mPath(wsPath),
    mProjectsPath(mPath.getPathTo("projects")),
    mDataPath(mPath.getPathTo(dataDirName)),
    mLibrariesPath(mDataPath.getPathTo("libraries")),
    mFileSystem(),
    mWorkspaceSettings(),
    mLibraryDb() {
  qDebug().nospace() << "Open workspace data directory " << mDataPath.toNative()
                     << "...";

  // Fail if the path is not a valid workspace directory.
  QString errorMsg;
  if (!checkCompatibility(mPath, &errorMsg)) {
    throw RuntimeError(__FILE__, __LINE__, errorMsg);
  }

  // Ensure that the projects directory exists since several features depend
  // on it.
  FileUtils::makePath(mProjectsPath);  // can throw

  // Access the data directory with TransactionalFileSystem to ensure a
  // failsafe file access and forbid concurrent access by a lock.
  mFileSystem = TransactionalFileSystem::openRW(
      mDataPath, TransactionalFileSystem::RestoreMode::yes,
      lockCallback);  // can throw

  // Check file format of data directory.
  QString versionFilePath = ".librepcb-data";
  Version loadedFileFormat = Version::fromString("0.1");
  if (mFileSystem->fileExists(versionFilePath)) {
    QByteArray raw(mFileSystem->read(versionFilePath));  // can throw
    VersionFile file = VersionFile::fromByteArray(raw);  // can throw
    loadedFileFormat = file.getVersion();
    if (loadedFileFormat > Application::getFileFormatVersion()) {
      throw LogicError(__FILE__, __LINE__,
                       QString("Workspace data directory requires LibrePCB %1 "
                               "or later to open..")
                           .arg(loadedFileFormat.toStr()));
    }
  }

  // Upgrade file format, if needed.
  TransactionalDirectory dataDir(mFileSystem);
  for (auto migration : FileFormatMigration::getMigrations(loadedFileFormat)) {
    qInfo().nospace().noquote()
        << "Workspace data file format is outdated, upgrading from v"
        << migration->getFromVersion().toStr() << " to v"
        << migration->getToVersion().toStr() << "...";
    migration->upgradeWorkspaceData(dataDir);
  }

  // Load workspace settings.
  mWorkspaceSettings.reset(new WorkspaceSettings(this));
  const QString settingsFilePath = "settings.lp";
  if (mFileSystem->fileExists(settingsFilePath)) {
    qDebug("Load workspace settings...");
    const std::unique_ptr<const SExpression> root =
        SExpression::parse(mFileSystem->read(settingsFilePath),
                           mFileSystem->getAbsPath(settingsFilePath));
    mWorkspaceSettings->load(*root, loadedFileFormat);
    qDebug("Successfully loaded workspace settings.");
  } else {
    qInfo("Workspace settings file not found, default settings will be used.");
  }

  // Write files to disk if an upgrade was performed.
  if (loadedFileFormat != Application::getFileFormatVersion()) {
    saveSettingsToTransactionalFileSystem();  // can throw
    mFileSystem->save();  // can throw
  }

  // Load library database.
  FileUtils::makePath(mLibrariesPath);  // can throw
  mLibraryDb.reset(new WorkspaceLibraryDb(mLibrariesPath));  // can throw

  // Done!
  qDebug("Successfully opened workspace.");
}

Workspace::~Workspace() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Workspace::saveSettings() {
  qDebug() << "Save workspace settings...";
  saveSettingsToTransactionalFileSystem();  // can throw
  mFileSystem->save();  // can throw
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool Workspace::checkCompatibility(const FilePath& wsRoot, QString* error) {
  // Check existence of version file.
  const FilePath versionFp = wsRoot.getPathTo(".librepcb-workspace");
  if (!versionFp.isExistingFile()) {
    if (error) {
      *error = tr("The directory \"%1\" is not a valid LibrePCB workspace.")
                   .arg(wsRoot.toNative());
    }
    return false;
  }

  // Check workspace file format.
  const QByteArray versionRaw(FileUtils::readFile(versionFp));  // can throw
  const VersionFile versionFile =
      VersionFile::fromByteArray(versionRaw);  // can throw
  if (versionFile.getVersion() != FILE_FORMAT_VERSION()) {
    if (error) {
      *error = tr("The workspace \"%1\" requires LibrePCB %2 or later.")
                   .arg(versionFile.getVersion().toStr());
    }
    return false;
  }

  return true;
}

QMap<QString, Version> Workspace::findDataDirectories(const FilePath& wsRoot) {
  QMap<QString, Version> result;
  const QDir dir(wsRoot.toStr());
  const QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
  foreach (const QString& subdir, dirs) {
    if (subdir == "data") {
      const FilePath path = wsRoot.getPathTo(subdir);
      const QString versionFile = ".librepcb-data";
      std::shared_ptr<TransactionalFileSystem> fs =
          TransactionalFileSystem::openRO(
              path, TransactionalFileSystem::RestoreMode::yes);
      if (fs->fileExists(versionFile)) {
        result.insert(
            subdir,
            VersionFile::fromByteArray(fs->read(versionFile)).getVersion());
      } else {
        // File format 0.1 didn't have a version file.
        result.insert(subdir, Version::fromString("0.1"));
      }
    } else if (subdir.startsWith('v')) {
      if (tl::optional<Version> v = Version::tryFromString(subdir.mid(1, -1))) {
        // IMPORTANT: Return the version number contained in the directory
        // name, *NOT* the version number of the file format contained within
        // that directory! The file format might be older, but the directory
        // is allowed/intended to be silently upgraded up to the file format
        // of the directory name.
        result.insert(subdir, *v);
      }
    }
  }
  return result;
}

QString Workspace::determineDataDirectory(
    const QMap<QString, Version>& dataDirs, QString& copyFromDir,
    QString& copyToDir) noexcept {
  const Version fileFormat = Application::getFileFormatVersion();
  const QString versionedDirName = "v" % fileFormat.toStr();
  const QString defaultDirName = "data";
  copyFromDir = QString();
  copyToDir = QString();

  // If there's a specific data directory for the current file format, use it.
  const auto versionedIt = dataDirs.find(versionedDirName);
  if (versionedIt != dataDirs.end()) {
    return versionedDirName;
  }

  // If the default data directory file format can be loaded, use it.
  const auto defaultIt = dataDirs.find(defaultDirName);
  if ((defaultIt != dataDirs.end()) && (defaultIt.value() <= fileFormat)) {
    // If the file format needs to be upgraded, a backup should be created.
    // But only if it doesn't exist yet, otherwise we can just do the upgrade.
    const QString backupDir = "v" % defaultIt->toStr();
    if ((defaultIt.value() < fileFormat) && (!dataDirs.contains(backupDir))) {
      copyFromDir = defaultDirName;
      copyToDir = backupDir;
    }
    return defaultDirName;
  }

  // There's no data directory to open, so we have to create a new one.
  const QString dataDir =
      (!dataDirs.contains(defaultDirName)) ? defaultDirName : versionedDirName;

  // If there are older file formats available, the latest one should be
  // imported.
  tl::optional<Version> versionToImport;
  for (auto it = dataDirs.begin(); it != dataDirs.end(); it++) {
    if ((it.key() != defaultDirName) && (it.value() < fileFormat) &&
        ((!versionToImport) || (it.value() > (*versionToImport)))) {
      copyFromDir = it.key();
      versionToImport = it.value();
    }
  }
  if (versionToImport) {
    copyToDir = dataDir;
  }
  return dataDir;
}

void Workspace::createNewWorkspace(const FilePath& path) {
  FileUtils::writeFile(
      path.getPathTo(".librepcb-workspace"),
      VersionFile(FILE_FORMAT_VERSION()).toByteArray());  // can throw
}

FilePath Workspace::getMostRecentlyUsedWorkspacePath() noexcept {
  QSettings clientSettings;
  return FilePath(
      clientSettings.value("workspaces/most_recently_used").toString());
}

void Workspace::setMostRecentlyUsedWorkspacePath(
    const FilePath& path) noexcept {
  QSettings clientSettings;
  clientSettings.setValue("workspaces/most_recently_used", path.toNative());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Workspace::saveSettingsToTransactionalFileSystem() {
  const std::unique_ptr<const SExpression> sexpr =
      mWorkspaceSettings->serialize();  // can throw
  mFileSystem->write("settings.lp",
                     sexpr->toByteArray());  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
