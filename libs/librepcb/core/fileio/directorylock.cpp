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
#include "directorylock.h"

#include "../systeminfo.h"
#include "fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DirectoryLock::DirectoryLock() noexcept
  : mDirToLock(), mLockFilePath(), mLockedByThisObject(false) {
  // nothing to do...
}

DirectoryLock::DirectoryLock(const FilePath& dir) noexcept
  : mDirToLock(), mLockFilePath(), mLockedByThisObject(false) {
  setDirToLock(dir);
}

DirectoryLock::~DirectoryLock() noexcept {
  try {
    unlockIfLocked();  // can throw
  } catch (const Exception& e) {
    qCritical() << "Could not remove lock file:" << e.getMsg();
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void DirectoryLock::setDirToLock(const FilePath& dir) noexcept {
  Q_ASSERT(!mLockedByThisObject);
  mDirToLock = dir;
  mLockFilePath = dir.getPathTo(".lock");
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

DirectoryLock::LockStatus DirectoryLock::getStatus(
    QString* lockedByUser) const {
  // check if the directory to lock does exist
  if (!mDirToLock.isExistingDir()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("The directory \"%1\" does not exist.").arg(mDirToLock.toNative()));
  }

  // when the directory is valid, the lock filepath must be valid too
  Q_ASSERT(mLockFilePath.isValid());

  // check if the lock file exists
  if (!mLockFilePath.isExistingFile()) {
    return LockStatus::Unlocked;
  }

  // read the content of the lock file
  QString content =
      QString::fromUtf8(FileUtils::readFile(mLockFilePath));  // can throw
  QStringList lines = content.split("\n", QString::KeepEmptyParts);
  // check count of lines
  if (lines.count() < 6) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The lock file \"%1\" has too few lines.")
                           .arg(mLockFilePath.toNative()));
  }
  // read lock metadata
  QString lockUser = lines.at(1);
  QString lockHost = lines.at(2);
  qint64 lockPid = lines.at(3).toLongLong();
  QString lockAppName = lines.at(4);
  if (lockedByUser) {
    *lockedByUser = lockUser % "@" % lockHost;
  }

  // read metadata about this application instance
  QString thisUser = SystemInfo::getUsername();
  QString thisHost = SystemInfo::getHostname();
  qint64 thisPid = QCoreApplication::applicationPid();

  // Check if the lock file was created with another computer or user.
  if ((lockUser != thisUser) || (lockHost != thisHost)) {
    return LockStatus::LockedByOtherUser;
  }

  // Check if the lock was created by this application instance.
  if (lockPid == thisPid) {
    if (dirsLockedByThisAppInstance().contains(mDirToLock)) {
      return LockStatus::LockedByThisApp;
    } else {
      // If the lock was created by the same PID as this instance but the lock
      // was not created by this instance, probably LibrePCB is running with a
      // namespaced PID (i.e. the same PID is used by multiple application
      // instances). This happens for example when running LibrePCB as a Flatpak
      // (see https://github.com/LibrePCB/LibrePCB/issues/734). Unfortunately in
      // this case we are not able to detect whether the lock is stale or not.
      return LockStatus::LockedByUnknownApp;
    }
  }

  // the lock file was created with another application instance on this
  // computer, now check if that process is still running (if not, the lock is
  // considered as stale)
  if (SystemInfo::isProcessRunning(lockPid) &&
      (SystemInfo::getProcessNameByPid(lockPid) == lockAppName)) {
    // The application which holds the lock is still running.
    return LockStatus::LockedByOtherApp;
  } else {
    // The process which created the lock is no longer running.
    return LockStatus::StaleLock;
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DirectoryLock::tryLock(LockHandlerCallback lockHandler) {
  QString user;
  LockStatus status = getStatus(&user);  // can throw
  switch (status) {
    case LockStatus::StaleLock:
      qWarning() << "Overriding stale lock on directory:" << mDirToLock;
      // fallthrough
    case LockStatus::Unlocked:
      lock();  // can throw
      break;
    default:  // Locked!
      if (lockHandler && lockHandler(mDirToLock, status, user)) {
        qInfo() << "Overriding lock on directory:" << mDirToLock;
        lock();  // can throw
        break;
      }
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Could not lock the directory \"%1\" because it is "
                            "already locked by \"%2\". Close any application "
                            "accessing this directory and try again.")
                             .arg(mDirToLock.toNative(), user));
  }
}

bool DirectoryLock::unlockIfLocked() {
  if (mLockedByThisObject) {
    unlock();  // can throw
    return true;
  } else {
    return false;
  }
}

void DirectoryLock::lock() {
  // check if the directory to lock does exist
  if (!mDirToLock.isExistingDir()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("The directory \"%1\" does not exist.").arg(mDirToLock.toNative()));
  }

  // when the directory is valid, the lock filepath must be valid too
  Q_ASSERT(mLockFilePath.isValid());

  // prepare the content which will be written to the lock file
  QStringList lines;
  lines.append(SystemInfo::getFullUsername());
  lines.append(SystemInfo::getUsername());
  lines.append(SystemInfo::getHostname());
  lines.append(QString::number(QCoreApplication::applicationPid()));
  lines.append(SystemInfo::getProcessNameByPid(qApp->applicationPid()));
  lines.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  QByteArray utf8content = lines.join('\n').toUtf8();

  // create/overwrite lock file
  FileUtils::writeFile(mLockFilePath, utf8content);  // can throw

  // File Lock successfully created
  mLockedByThisObject = true;
  dirsLockedByThisAppInstance().insert(mDirToLock);
}

void DirectoryLock::unlock() {
  // remove the lock file
  FileUtils::removeFile(mLockFilePath);  // can throw

  // File Lock successfully removed
  mLockedByThisObject = false;
  dirsLockedByThisAppInstance().remove(mDirToLock);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QSet<FilePath>& DirectoryLock::dirsLockedByThisAppInstance() noexcept {
  static QSet<FilePath> set;
  return set;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
