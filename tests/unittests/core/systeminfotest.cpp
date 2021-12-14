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

#include <gtest/gtest.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/systeminfo.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SystemInfoTest : public ::testing::Test {
protected:
  FilePath getTestProcessExePath() const noexcept {
    FilePath generatedDir(qApp->applicationDirPath());
#if defined(Q_OS_UNIX) || defined(Q_OS_OSX)  // UNIX/Linux or macOS
    return generatedDir.getPathTo("dummy-binary");
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
    return generatedDir.getPathTo("dummy-binary.exe");
#else
#error "Unknown operating system!"
#endif
  }

  QString getOwnProcessExeName() const noexcept {
#if defined(Q_OS_SOLARIS)
    // Note: Solaris limits process names to 15 bytes
    return QString("librepcb-unitte");
#elif defined(Q_OS_OPENBSD)
    // Note: OpenBSD limits process names to 16 bytes
    return QString("librepcb-unittes");
#else
    return QString("librepcb-unittests");
#endif
  }

  QString getTestProcessExeName() const noexcept {
    return QString("dummy-binary");
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(SystemInfoTest, testGetUsername) {
  // the username must not be empty on any system
  QString username = SystemInfo::getUsername();
  EXPECT_FALSE(username.isEmpty());
  std::cout << "Username: " << qPrintable(username) << std::endl;
}

TEST_F(SystemInfoTest, testGetFullUsername) {
  // the full username may be empty because the user didn't set it...
  QString fullUsername = SystemInfo::getFullUsername();
  std::cout << "Full username: " << qPrintable(fullUsername) << std::endl;
}

TEST_F(SystemInfoTest, testGetHostname) {
  // the hostname must not be empty on any system
  QString hostname = SystemInfo::getHostname();
  EXPECT_FALSE(hostname.isEmpty());
  std::cout << "Hostname: " << qPrintable(hostname) << std::endl;
}

TEST_F(SystemInfoTest, testIsProcessRunning) {
  // check this process
  { EXPECT_TRUE(SystemInfo::isProcessRunning(qApp->applicationPid())); }

  // check another running process
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

  // check an invalid process
  { EXPECT_FALSE(SystemInfo::isProcessRunning(999999)); }
}

TEST_F(SystemInfoTest, testGetProcessNameByPid) {
  // check this process
  {
    QString processName =
        SystemInfo::getProcessNameByPid(qApp->applicationPid());
    EXPECT_EQ(getOwnProcessExeName(), processName) << qPrintable(processName);
  }

  // check another running process
  {
    QProcess process;
    process.start(getTestProcessExePath().toStr());
    bool success = process.waitForStarted();
    ASSERT_TRUE(success) << qPrintable(process.errorString());
    qint64 pid = process.processId();
    ASSERT_NE(pid, qApp->applicationPid());
    // the next line is an ugly workaround for infrequent test failures on Mac
    // OS X
    QThread::msleep(200);
    QThread::yieldCurrentThread();
    QString processName = SystemInfo::getProcessNameByPid(pid);
    EXPECT_EQ(getTestProcessExeName(), processName) << qPrintable(processName);
    process.kill();
    success = process.waitForFinished();
    ASSERT_TRUE(success) << qPrintable(process.errorString());
    // the next line is an ugly workaround for infrequent test failures on Mac
    // OS X
    QThread::msleep(200);
    QThread::yieldCurrentThread();
    processName = SystemInfo::getProcessNameByPid(pid);
    EXPECT_EQ(QString(), processName) << qPrintable(processName);
  }

  // check an invalid process
  {
    QString processName = SystemInfo::getProcessNameByPid(999999);
    EXPECT_EQ(QString(), processName) << qPrintable(processName);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
