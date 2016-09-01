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
#include <librepcbcommon/systeminfo.h>
#include <librepcbcommon/fileio/filepath.h>

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

        FilePath getFilepathToLibrepcbExecutable() const noexcept
        {
            FilePath generatedDir(qApp->applicationDirPath());
#if defined(Q_OS_OSX) // Mac OS X
            return generatedDir.getPathTo("librepcb.app/Contents/MacOS/librepcb");
#elif defined(Q_OS_UNIX) // UNIX/Linux
            return generatedDir.getPathTo("librepcb");
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64) // Windows
            return generatedDir.getPathTo("librepcb.exe");
#else
#error "Unknown operating system!"
#endif
        }
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(SystemInfoTest, testGetUsername)
{
    // the username must not be empty on any system
    QString username = SystemInfo::getUsername();
    ASSERT_FALSE(username.isEmpty());
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
    ASSERT_FALSE(hostname.isEmpty());
    std::cout << "Hostname: " << qPrintable(hostname) << std::endl;
}

TEST_F(SystemInfoTest, testIsProcessRunning)
{
    // check this process
    {
        ASSERT_TRUE(SystemInfo::isProcessRunning(qApp->applicationPid()));
    }

    // check another running process
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)) // QProcess::processId() requires Qt>=5.3
    {
        QProcess process;
        process.start(getFilepathToLibrepcbExecutable().toStr());
        bool success = process.waitForStarted();
        if (!success) {
            std::cout << "Failed to start process: " << qPrintable(process.errorString())
                      << "[" << process.error() << "]" << std::endl;
            GTEST_FAIL();
        }
        ASSERT_TRUE(SystemInfo::isProcessRunning(process.processId()));
        process.kill();
        success = process.waitForFinished();
        if (!success) {
            std::cout << "Failed to stop process: " << qPrintable(process.errorString())
                      << "[" << process.error() << "]" << std::endl;
            GTEST_FAIL();
        }
    }
#endif

    // check an invalid process
    {
        ASSERT_FALSE(SystemInfo::isProcessRunning(999999));
    }
}

TEST_F(SystemInfoTest, testGetProcessNameByPid)
{
    // check this process
    {
        ASSERT_EQ("tests", SystemInfo::getProcessNameByPid(qApp->applicationPid()));
    }

    // check another running process
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)) // QProcess::processId() requires Qt>=5.3
    {
        QProcess process;
        process.start(getFilepathToLibrepcbExecutable().toStr());
        bool success = process.waitForStarted();
        if (!success) {
            std::cout << "Failed to start process: " << qPrintable(process.errorString())
                      << "[" << process.error() << "]" << std::endl;
            GTEST_FAIL();
        }
        QThread::usleep(1); // GetModuleFileNameEx() fails without this tiny delay o_o
        ASSERT_EQ("librepcb", SystemInfo::getProcessNameByPid(process.processId()));
        process.kill();
        if (!success) {
            std::cout << "Failed to stop process: " << qPrintable(process.errorString())
                      << "[" << process.error() << "]" << std::endl;
            GTEST_FAIL();
        }
    }
#endif

    // check an invalid process
    {
        ASSERT_EQ(QString(), SystemInfo::getProcessNameByPid(999999));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
