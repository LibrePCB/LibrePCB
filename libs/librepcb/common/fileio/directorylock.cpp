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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "directorylock.h"
#include "fileutils.h"
#include "../systeminfo.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DirectoryLock::DirectoryLock() noexcept :
    mDirToLock(), mLockFilePath(), mLockedByThisObject(false)
{
    // nothing to do...
}

DirectoryLock::DirectoryLock(const FilePath& dir) noexcept :
    mDirToLock(), mLockFilePath(), mLockedByThisObject(false)
{
    setDirToLock(dir);
}

DirectoryLock::~DirectoryLock() noexcept
{
    if (mLockedByThisObject) {
        try {
            unlock(); // can throw
        } catch (const Exception& e) {
            qCritical() << "Could not remove lock file:" << e.getUserMsg();
        }
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void DirectoryLock::setDirToLock(const FilePath& dir) noexcept
{
    Q_ASSERT(!mLockedByThisObject);
    mDirToLock = dir;
    mLockFilePath = dir.getPathTo(".lock");
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

DirectoryLock::LockStatus DirectoryLock::getStatus() const throw (Exception)
{
    // check if the directory to lock does exist
    if (!mDirToLock.isExistingDir()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),  QString(
            tr("The directory \"%1\" does not exist.")).arg(mDirToLock.toNative()));
    }

    // when the directory is valid, the lock filepath must be valid too
    Q_ASSERT(mLockFilePath.isValid());

    // check if the lock file exists
    if (!mLockFilePath.isExistingFile()) {
        return LockStatus::Unlocked;
    }

    // read the content of the lock file
    QString content = QString::fromUtf8(FileUtils::readFile(mLockFilePath)); // can throw
    QStringList lines = content.split("\n", QString::KeepEmptyParts);
    // check count of lines
    if (lines.count() < 6) {
        throw RuntimeError(__FILE__, __LINE__, content, QString(tr(
            "The lock file \"%1\" has too few lines.")).arg(mLockFilePath.toNative()));
    }
    // read lock metadata
    QString lockUser = lines.at(1);
    QString lockHost = lines.at(2);
    qint64 lockPid = lines.at(3).toLongLong();
    QString lockAppName = lines.at(4);

    // read metadata about this application instance
    QString thisUser = SystemInfo::getUsername();
    QString thisHost = SystemInfo::getHostname();

    // check if the lock file was created with another computer or user
    if ((lockUser != thisUser) || (lockHost != thisHost)) {
        return LockStatus::Locked;
    }

    // the lock file was created with an application instance on this computer, now check
    // if that process is still running (if not, the lock is considered as stale)
    if (SystemInfo::isProcessRunning(lockPid)) {
        if (SystemInfo::getProcessNameByPid(lockPid) == lockAppName) {
            return LockStatus::Locked; // the application which holds the lock is still running
        } else {
            return LockStatus::StaleLock; // the process which holds the lock seems to be crashed
        }
    } else {
        // the process which holds the lock is no longer running
        return LockStatus::StaleLock;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void DirectoryLock::lock() throw (Exception)
{
    // check if the directory to lock does exist
    if (!mDirToLock.isExistingDir()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),  QString(
            tr("The directory \"%1\" does not exist.")).arg(mDirToLock.toNative()));
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
    FileUtils::writeFile(mLockFilePath, utf8content); // can throw

    // File Lock successfully created
    mLockedByThisObject = true;
}

void DirectoryLock::unlock() throw (Exception)
{
    // remove the lock file
    FileUtils::removeFile(mLockFilePath); // can throw

    // File Lock successfully removed
    mLockedByThisObject = false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
