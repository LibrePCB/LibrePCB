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
#include <librepcb/common/systeminfo.h>
#include <librepcb/common/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class SystemInfoTest : public ::testing::Test
{
    protected:

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

        QString getTestProcessExeName() const noexcept
        {
            return QString("uuid-generator");
        }
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(SystemInfoTest, testGetUsername)
{
    // the username must not be empty on any system
    QString username = SystemInfo::getUsername();
    EXPECT_FALSE(username.isEmpty());
    std::cout << "Username: " << qPrintable(username) << std::endl;
}

TEST_F(SystemInfoTest, testGetFullUsername)
{
    // the full username may be empty because the user didn't set it...
    QString fullUsername = SystemInfo::getFullUsername();
    std::cout << "Full username: " << qPrintable(fullUsername) << std::endl;
}

TEST_F(SystemInfoTest, testGetHostname)
{
    // the hostname must not be empty on any system
    QString hostname = SystemInfo::getHostname();
    EXPECT_FALSE(hostname.isEmpty());
    std::cout << "Hostname: " << qPrintable(hostname) << std::endl;
}

TEST_F(SystemInfoTest, testIsProcessRunning)
{
    // check this process
    {
        EXPECT_TRUE(SystemInfo::isProcessRunning(qApp->applicationPid()));
    }

    // check another running process
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)) // QProcess::processId() requires Qt>=5.3
    {
        QProcess process;
        process.start(getTestProcessExePath().toStr());
        bool success = process.waitForStarted();
        ASSERT_TRUE(success) << qPrintable(process.errorString());
        qint64 pid = process.processId();
        EXPECT_TRUE(SystemInfo::isProcessRunning(pid));
        process.kill();
        success = process.waitForFinished();
        ASSERT_TRUE(success) << qPrintable(process.errorString());
        EXPECT_FALSE(SystemInfo::isProcessRunning(pid));
    }
#endif

    // check an invalid process
    {
        EXPECT_FALSE(SystemInfo::isProcessRunning(999999));
    }
}

TEST_F(SystemInfoTest, testGetProcessNameByPid)
{
    // check this process
    {
        QString processName = SystemInfo::getProcessNameByPid(qApp->applicationPid());
        EXPECT_EQ("librepcb-unittests", processName) << qPrintable(processName);
    }

    // check another running process
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)) // QProcess::processId() requires Qt>=5.3
    {
        QProcess process;
        process.start(getTestProcessExePath().toStr());
        bool success = process.waitForStarted();
        ASSERT_TRUE(success) << qPrintable(process.errorString());
        qint64 pid = process.processId();
        ASSERT_NE(pid, qApp->applicationPid());
        // the next line is an ugly workaround for infrequent test failures on Mac OS X
        QThread::msleep(200); QThread::yieldCurrentThread();
        QString processName = SystemInfo::getProcessNameByPid(pid);
        EXPECT_EQ(getTestProcessExeName(), processName) << qPrintable(processName);
        process.kill();
        success = process.waitForFinished();
        ASSERT_TRUE(success) << qPrintable(process.errorString());
        // the next line is an ugly workaround for infrequent test failures on Mac OS X
        QThread::msleep(200); QThread::yieldCurrentThread();
        processName = SystemInfo::getProcessNameByPid(pid);
        EXPECT_EQ(QString(), processName) << qPrintable(processName);
    }
#endif

    // check an invalid process
    {
        QString processName = SystemInfo::getProcessNameByPid(999999);
        EXPECT_EQ(QString(), processName) << qPrintable(processName);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
