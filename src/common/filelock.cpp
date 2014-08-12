/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "filelock.h"
#include "systeminfo.h"
#include "filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FileLock::FileLock() noexcept :
    mLockFilepath(), mLockedByThisObject(false)
{
    // nothing to do...
}

FileLock::FileLock(const FilePath& filepath) noexcept :
    mLockFilepath(), mLockedByThisObject(false)
{
    setFileToLock(filepath);
}

FileLock::~FileLock() noexcept
{
    if (mLockedByThisObject)
        unlock();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool FileLock::setFileToLock(const FilePath& filepath) noexcept
{
    if (mLockedByThisObject)
    {
        qCritical() << "it's not allowed to set a new filepath while the lock is active!!";
        return false;
    }

    // determine the filepath for the lock file
    QString lockFileName = QStringLiteral(".~lock.") % filepath.getFilename() % QStringLiteral("#");
    mLockFilepath = filepath.getParentDir().getPathTo(lockFileName);

    if (!mLockFilepath.isValid())
    {
        qCritical() << "invalid lock filepath!";
        return false;
    }

    return true;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const FilePath& FileLock::getLockFilepath() const noexcept
{
    if (!mLockFilepath.isValid())
        qCritical() << "getLockFilepath() was called before setFileToLock()!";

    return mLockFilepath;
}

FileLock::LockStatus FileLock::getStatus() const noexcept
{
    if (!mLockFilepath.isValid())
    {
        qCritical() << "getStatus() was called before setFileToLock()!";
        return Error;
    }

    QFile file(mLockFilepath.toStr());

    if (!file.exists())
    {
        // there is no lock file
        return Unlocked;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Could not open the lock file:" << mLockFilepath.toStr();
        return Error;
    }

    // read the content of the lock file
    QStringList entries = QString::fromUtf8(file.readAll()).split(",");
    file.close();

    if (entries.count() < 5)
    {
        qWarning() << "Invalid lock file content:" << entries;
        return Error;
    }

    // check who has created the lock file
    // (remove all commas as they are not allowed in the comma-seperated list)
    if (       (entries[0] == SystemInfo::getFullUsername().remove(","))
            && (entries[1] == SystemInfo::getUsername().remove(","))
            && (entries[2] == SystemInfo::getHostname().remove(",")))
    {
        // the lock file was created by the current user and host computer
        // TODO: check the PID in the lock file to determine whether that application
        // instance was crashed or is still running!
        return StaleLock;
    }
    else
    {
        // the lock file was created by another application instance
        return Locked;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool FileLock::lock() noexcept
{
    if (!mLockFilepath.isValid())
    {
        qCritical() << "lock() was called before setFileToLock()!";
        return false;
    }

    // prepare the content which will be written to the lock file
    // (remove all commas as they are not allowed in the comma-seperated list)
    QString content = QString("%1,%2,%3,%4,%5").arg(
                          SystemInfo::getFullUsername().remove(","),
                          SystemInfo::getUsername().remove(","),
                          SystemInfo::getHostname().remove(","),
                          QString::number(QCoreApplication::applicationPid()),
                          QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    // create all parent directories (if they do not exist)
    if (!mLockFilepath.getParentDir().mkPath())
    {
        qWarning() << "Could not create directories of the path" << mLockFilepath.toStr();
        // do not return here; if the directories do really not exist, writing to the
        // lock file will also abort...
        //return false;
    }

    QFile file(mLockFilepath.toStr());
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not open the lock file:" << mLockFilepath.toStr();
        return false;
    }

    QByteArray utf8content = content.toUtf8();
    if (file.write(utf8content) != utf8content.length())
    {
        qWarning() << "Could not write to the lock file:" << mLockFilepath.toStr();
        file.close();
        return false;
    }

    file.close();
    mLockedByThisObject = true;
    return true;
}

bool FileLock::unlock() noexcept
{
    if (!mLockFilepath.isValid())
    {
        qCritical() << "unlock() was called before setFileToLock()!";
        return false;
    }

    QFile file(mLockFilepath.toStr());
    if (file.exists())
    {
        if (!file.remove())
        {
            qWarning() << "Could not remove the lock file:" << mLockFilepath.toStr();
            return false;
        }
    }

    mLockedByThisObject = false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
