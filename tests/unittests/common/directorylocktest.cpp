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
#include <gtest/gtest.h>
#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/systeminfo.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class DirectoryLockTest : public ::testing::Test
{
    protected:

        virtual void SetUp() override
        {
            // create temporary, empty directory
            mTempDir = FilePath::getApplicationTempPath().getPathTo("DirectoryLockTest");
            mTempLockFilePath = FilePath(mTempDir.toStr() % "/.lock");
            if (mTempDir.isExistingDir()) {
                FileUtils::removeDirRecursively(mTempDir); // can throw
            }
            FileUtils::makePath(mTempDir);
        }

        virtual void TearDown() override
        {
            // remove temporary directory
            FileUtils::removeDirRecursively(mTempDir); // can throw
        }

        FilePath getTestProcessExePath() const noexcept
        {
            FilePath generatedDir(qApp->applicationDirPath());
#if defined(Q_OS_OSX) // Mac OS X
            return generatedDir.getPathTo("uuid-generator.app/Contents/MacOS/uuid-generator");
#elif defined(Q_OS_UNIX) // UNIX/Linux
            return generatedDir.getPathTo("uuid-generator");
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64) // Windows
            return generatedDir.getPathTo("uuid-generator.exe");
#else
#error "Unknown operating system!"
#endif
        }

        FilePath mTempDir;
        FilePath mTempLockFilePath;
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(DirectoryLockTest, testDefaultConstructor)
{
    DirectoryLock lock;
    EXPECT_FALSE(lock.getDirToLock().isValid());
    EXPECT_FALSE(lock.getLockFilepath().isValid());
    EXPECT_THROW(lock.getStatus(), Exception);
    EXPECT_THROW(lock.tryLock(), Exception);
    EXPECT_THROW(lock.lock(), Exception);
    EXPECT_THROW(lock.unlock(), Exception);
}

TEST_F(DirectoryLockTest, testConstructorWithExistingDir)
{
    DirectoryLock lock(mTempDir);
    EXPECT_EQ(mTempDir, lock.getDirToLock());
    EXPECT_EQ(mTempLockFilePath, lock.getLockFilepath());
    EXPECT_NO_THROW(lock.getStatus());
    EXPECT_NO_THROW(lock.tryLock());
    EXPECT_NO_THROW(lock.unlockIfLocked());
    EXPECT_NO_THROW(lock.lock());
    EXPECT_NO_THROW(lock.unlock());
}

TEST_F(DirectoryLockTest, testConstructorWithNonExistingDir)
{
    // using DirectoryLock on non-existent directories must not be possible
    FilePath dir = mTempDir.getPathTo("ghost");
    DirectoryLock lock(dir);
    EXPECT_EQ(dir, lock.getDirToLock());
    EXPECT_EQ(dir.getPathTo(".lock"), lock.getLockFilepath());
    EXPECT_THROW(lock.getStatus(), Exception);
    EXPECT_THROW(lock.tryLock(), Exception);
    EXPECT_THROW(lock.lock(), Exception);
    EXPECT_THROW(lock.unlock(), Exception);
}

TEST_F(DirectoryLockTest, testConstructorWithExistingFile)
{
    // create an empty file
    FilePath file = mTempDir.getPathTo("file");
    FileUtils::writeFile(file, QByteArray());

    // using DirectoryLock on a existing file (instead of a directory) must not be possible
    DirectoryLock lock(file);
    EXPECT_EQ(file, lock.getDirToLock());
    EXPECT_EQ(file.getPathTo(".lock"), lock.getLockFilepath());
    EXPECT_THROW(lock.getStatus(), Exception);
    EXPECT_THROW(lock.tryLock(), Exception);
    EXPECT_THROW(lock.lock(), Exception);
    EXPECT_THROW(lock.unlock(), Exception);
}

TEST_F(DirectoryLockTest, testDestructorUnlock)
{
    // destroying without lock
    {
        DirectoryLock lock(mTempDir);
    }
    EXPECT_FALSE(mTempLockFilePath.isExistingFile());

    // destroying after releasing lock
    {
        DirectoryLock lock(mTempDir);
        lock.lock();
        lock.unlock();
    }
    EXPECT_FALSE(mTempLockFilePath.isExistingFile());

    // destroying with active lock
    {
        DirectoryLock lock(mTempDir);
        lock.lock();
    }
    EXPECT_FALSE(mTempLockFilePath.isExistingFile());
}

TEST_F(DirectoryLockTest, testDestructorDontUnlock)
{
    // destroying without lock
    {
        DirectoryLock lock(mTempDir);
        FileUtils::writeFile(mTempLockFilePath, QByteArray()); // create imaginary lock file
    }
    EXPECT_TRUE(mTempLockFilePath.isExistingFile());

    // destroying after releasing lock
    {
        DirectoryLock lock(mTempDir);
        lock.lock();
        lock.unlock();
        FileUtils::writeFile(mTempLockFilePath, QByteArray()); // create imaginary lock file
    }
    EXPECT_TRUE(mTempLockFilePath.isExistingFile());
}

TEST_F(DirectoryLockTest, testSetGetDirToLock)
{
    // create invalid lock object
    DirectoryLock lock;
    EXPECT_EQ(FilePath(), lock.getDirToLock());
    EXPECT_EQ(FilePath(), lock.getLockFilepath());

    // set path and readback
    lock.setDirToLock(mTempDir);
    EXPECT_EQ(mTempDir, lock.getDirToLock());
    EXPECT_EQ(mTempLockFilePath, lock.getLockFilepath());
}

TEST_F(DirectoryLockTest, testSingleStatusLockUnlock)
{
    DirectoryLock lock(mTempDir);
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock.getStatus());

    // get lock
    lock.lock();
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock.getStatus());
    EXPECT_TRUE(mTempLockFilePath.isExistingFile());

    // release lock
    lock.unlock();
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock.getStatus());
    EXPECT_FALSE(mTempLockFilePath.isExistingFile());
}

TEST_F(DirectoryLockTest, testMultipleStatusLockUnlock)
{
    DirectoryLock lock1(mTempDir);
    DirectoryLock lock2(mTempDir);
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock1.getStatus());
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock2.getStatus());

    // get lock1
    lock1.lock();
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock1.getStatus());
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock2.getStatus());
    EXPECT_TRUE(mTempLockFilePath.isExistingFile());

    // get lock2 (steals the lock from lock1)
    lock2.lock();
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock1.getStatus());
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock2.getStatus());
    EXPECT_TRUE(mTempLockFilePath.isExistingFile());

    // release lock2
    lock2.unlock();
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock1.getStatus());
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock2.getStatus());
    EXPECT_FALSE(mTempLockFilePath.isExistingFile());
}

TEST_F(DirectoryLockTest, testTryLockWithoutArgument)
{
    DirectoryLock lock(mTempDir);
    lock.tryLock();
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock.getStatus());
}

TEST_F(DirectoryLockTest, testTryLockUnlockedDir)
{
    bool wasStale = true;
    DirectoryLock lock(mTempDir);
    lock.tryLock(&wasStale);
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock.getStatus());
    EXPECT_FALSE(wasStale);
}

TEST_F(DirectoryLockTest, testTryLockLockedDir)
{
    bool wasStale = true;
    DirectoryLock lock1(mTempDir);
    DirectoryLock lock2(mTempDir);
    lock1.tryLock(&wasStale);
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock1.getStatus());
    EXPECT_THROW(lock2.tryLock(), Exception);
    EXPECT_FALSE(wasStale);
}

TEST_F(DirectoryLockTest, testUnlockIfLockedOnUnlockedDir)
{
    DirectoryLock lock(mTempDir);
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock.getStatus());
    EXPECT_FALSE(lock.unlockIfLocked());
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock.getStatus());
}

TEST_F(DirectoryLockTest, testUnlockIfLockedOnLockedDir)
{
    DirectoryLock lock(mTempDir);
    lock.lock();
    EXPECT_EQ(DirectoryLock::LockStatus::Locked, lock.getStatus());
    EXPECT_TRUE(lock.unlockIfLocked());
    EXPECT_EQ(DirectoryLock::LockStatus::Unlocked, lock.getStatus());
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)) // QProcess::processId() requires Qt>=5.3
TEST_F(DirectoryLockTest, testStaleLock)
{
    QProcess process;
    process.start(getTestProcessExePath().toStr());
    bool success = process.waitForStarted();
    ASSERT_TRUE(success) << qPrintable(process.errorString());
    qint64 pid = process.processId();
    process.kill();
    success = process.waitForFinished();
    ASSERT_TRUE(success) << qPrintable(process.errorString());

    // get the lock
    DirectoryLock lock(mTempDir);
    lock.lock();

    // replace the PID in the lock file
    QStringList lines = QString(FileUtils::readFile(mTempLockFilePath)).split('\n');
    lines[3] = QString::number(pid);
    FileUtils::writeFile(mTempLockFilePath, lines.join('\n').toUtf8());

    // check status
    EXPECT_EQ(DirectoryLock::LockStatus::StaleLock, lock.getStatus());

    // try to get the lock
    bool wasStale = false;
    lock.tryLock(&wasStale);
    ASSERT_TRUE(wasStale);
}
#endif

TEST_F(DirectoryLockTest, testLockFileContent)
{
    // get the lock
    DirectoryLock lock(mTempDir);
    lock.lock();

    // read the lock file
    QList<QByteArray> lines = FileUtils::readFile(mTempLockFilePath).split('\n');

    // verify content
    EXPECT_EQ(6, lines.count());
    EXPECT_EQ(SystemInfo::getFullUsername(), QString(lines.value(0)));
    EXPECT_EQ(SystemInfo::getUsername(), QString(lines.value(1)));
    EXPECT_EQ(SystemInfo::getHostname(), QString(lines.value(2)));
    EXPECT_EQ(QString::number(qApp->applicationPid()), QString(lines.value(3)));
    EXPECT_EQ(SystemInfo::getProcessNameByPid(qApp->applicationPid()), QString(lines.value(4)));
    QDateTime lockTime = QDateTime::fromString(QString(lines.value(5)), Qt::ISODate);
    EXPECT_TRUE(lockTime.isValid());
    EXPECT_NEAR(lockTime.toMSecsSinceEpoch(),
                QDateTime::currentDateTime().toMSecsSinceEpoch(),
                10000); // allow up to 10 seconds difference
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
