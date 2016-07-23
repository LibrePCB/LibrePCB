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
#include "filelock.h"
#include "../systeminfo.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FileLock::FileLock() noexcept :
    mFileToLock(), mLockFilepath(), mLockedByThisObject(false)
{
    // nothing to do...
}

FileLock::FileLock(const FilePath& filepath) noexcept :
    mFileToLock(), mLockFilepath(), mLockedByThisObject(false)
{
    setFileToLock(filepath);
}

FileLock::~FileLock() noexcept
{
    if (mLockedByThisObject)
        try{unlock();} catch (...) {}
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void FileLock::setFileToLock(const FilePath& filepath) noexcept
{
    Q_ASSERT(!mLockedByThisObject);

    mFileToLock = filepath;

    // determine the filepath for the lock file
    QString lockFileName = QStringLiteral(".~lock.") % filepath.getFilename() % QStringLiteral("#");
    mLockFilepath = filepath.getParentDir().getPathTo(lockFileName);
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FileLock::LockStatus_t FileLock::getStatus() const throw (Exception)
{
    if (!mLockFilepath.isValid())
    {
        throw RuntimeError(__FILE__, __LINE__, mLockFilepath.toStr(),
            QString(tr("Invalid lock filepath: \"%1\"")).arg(mLockFilepath.toNative()));
    }

    QFile file(mLockFilepath.toStr());

    if (!file.exists())
    {
        // there is no lock file
        return LockStatus_t::Unlocked;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3]")
            .arg(mLockFilepath.toStr(), file.errorString()).arg(file.error()),
            QString(tr("Could not open file \"%1\": %2"))
            .arg(mLockFilepath.toNative(), file.errorString()));
    }

    // read the content of the lock file
    QStringList entries = QString::fromUtf8(file.readAll()).split(",");
    file.close();

    if (entries.count() < 5)
    {
        throw RuntimeError(__FILE__, __LINE__, mLockFilepath.toStr(),
            QString(tr("Invalid lock file \"%1\":\n%2"))
            .arg(mLockFilepath.toNative()).arg(entries.join(",")));
    }

    // check who has created the lock file
    // (remove all commas as they are not allowed in the comma-seperated list)
    if (       (entries[0] == SystemInfo::getFullUsername().remove(","))
            && (entries[1] == SystemInfo::getUsername().remove(","))
            && (entries[2] == SystemInfo::getHostname().remove(",")))
    {
        // the lock file was created by the current user and host computer -> check PID
        qint64 pid = 0;
        if (sizeof(qint64) == sizeof(int))
            pid = entries[3].toInt();
        else if (sizeof(qint64) == sizeof(long))
            pid = entries[3].toLong();
        else if (sizeof(qint64) == sizeof(long long))
            pid = entries[3].toLongLong();
        if (pid == 0)
        {
            throw LogicError(__FILE__, __LINE__, QString("%1/%2/%3")
                .arg(sizeof(qint64)).arg(sizeof(int)).arg(sizeof(long)));
        }

        if (pid == QCoreApplication::applicationPid())
        {
            // the lock file was created with this application instance
            return LockStatus_t::Locked;
        }
        else
        {
            // the lock file was created with another application instance
            // TODO: check if the application instance that has created the lock file is still running!
            return LockStatus_t::StaleLock;
        }
    }
    else
    {
        // the lock file was created by another application instance
        return LockStatus_t::Locked;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FileLock::lock() throw (Exception)
{
    if (!mLockFilepath.isValid())
    {
        throw RuntimeError(__FILE__, __LINE__, mLockFilepath.toStr(),
            QString(tr("Invalid lock filepath: \"%1\"")).arg(mLockFilepath.toNative()));
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
        // do not return here; if the directories do really do not exist, writing to the
        // lock file will also abort...
    }

    QSaveFile file(mLockFilepath.toStr());
    if (!file.open(QIODevice::WriteOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3]")
            .arg(mLockFilepath.toStr(), file.errorString()).arg(file.error()),
            QString(tr("Could not open or create file \"%1\": %2"))
            .arg(mLockFilepath.toNative(), file.errorString()));
    }

    QByteArray utf8content = content.toUtf8();
    qint64 written = file.write(utf8content);
    if (written != utf8content.size())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1: %2 (only %3 of %4 bytes written)")
            .arg(mLockFilepath.toStr(), file.errorString()).arg(written).arg(utf8content.size()),
            QString(tr("Could not write to file \"%1\": %2"))
            .arg(mLockFilepath.toNative(), file.errorString()));
    }

    if (!file.commit())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr("Could not write to "
            "file \"%1\": %2")).arg(mLockFilepath.toNative(), file.errorString()));
    }

    // File Lock successfully created
    mLockedByThisObject = true;
}

void FileLock::unlock() throw (Exception)
{
    if (!mLockFilepath.isValid())
    {
        throw RuntimeError(__FILE__, __LINE__, mLockFilepath.toStr(),
            QString(tr("Invalid lock filepath: \"%1\"")).arg(mLockFilepath.toNative()));
    }

    QFile file(mLockFilepath.toStr());
    if (file.exists())
    {
        if (!file.remove())
        {
            throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3]")
                .arg(mLockFilepath.toStr(), file.errorString()).arg(file.error()),
                QString(tr("Could not remove file \"%1\": %2"))
                .arg(mLockFilepath.toNative(), file.errorString()));
        }
    }

    // File Lock successfully removed
    mLockedByThisObject = false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
