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
#include "systeminfo.h"

#include "exceptions.h"
#include "fileio/filepath.h"

#include <QHostInfo>
#include <QtCore>

#if defined(Q_OS_OSX)  // macOS
#include <sys/types.h>
#include <system_error>

#include <cerrno>
#include <libproc.h>
#include <signal.h>
#elif defined(Q_OS_UNIX)  // UNIX/Linux
#include <sys/types.h>
#include <system_error>

#include <cerrno>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#if defined(Q_OS_SOLARIS)
#include <libproc.h>
#endif
#if defined(Q_OS_OPENBSD)
#include <sys/sysctl.h>
#endif
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
#ifdef WINVER
#undef WINVER
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <windows.h>
#else
#error "Unknown operating system!"
#endif

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

const QString& SystemInfo::getUsername() noexcept {
  auto get = []() {
    // this line should work for most UNIX, Linux, Mac and Windows systems
    QString s =
        QString(qgetenv("USERNAME")).remove('\n').remove('\r').trimmed();

    // if the environment variable "USERNAME" is not set, we will try "USER"
    if (s.isEmpty()) {
      s = QString(qgetenv("USER")).remove('\n').remove('\r').trimmed();
    }

    if (s.isEmpty()) {
      qWarning() << "Could not determine the system's username!";
    }

    return s;
  };

  static const QString value = get();  // Thread-safe initialization.
  return value;
}

const QString& SystemInfo::getFullUsername() noexcept {
  auto get = []() {
    QString s;
#if defined(Q_OS_OSX)  // macOS
    QString command(
        "finger `whoami` | awk -F: '{ print $3 }' | head -n1 | sed 's/^ //'");
    QProcess process;
    process.start("sh", QStringList() << "-c" << command);
    process.waitForFinished(500);
    s = QString(process.readAllStandardOutput())
            .remove('\n')
            .remove('\r')
            .trimmed();
#elif defined(Q_OS_UNIX)  // UNIX/Linux
    passwd* userinfo = getpwuid(getuid());
    if (userinfo == NULL) {
      qWarning() << "Could not fetch user info via getpwuid!";
    } else {
      QString gecosString = QString::fromLocal8Bit(userinfo->pw_gecos);
      s = gecosString.section(',', 0, 0).remove('\n').remove('\r').trimmed();
    }
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
    QString command("net user %USERNAME%");
    QProcess process;
    process.start("cmd", QStringList() << "/c" << command);
    process.waitForFinished(500);
    QStringList lines = QString(process.readAllStandardOutput()).split('\n');
    foreach (const QString& line, lines) {
      if (line.contains("Full Name")) {
        s = QString(line)
                .remove("Full Name")
                .remove('\n')
                .remove('\r')
                .trimmed();
        break;
      }
    }
#else
#error "Unknown operating system!"
#endif

    if (s.isEmpty()) {
      qWarning()
          << "The system's full username is empty or could not be determined!";
      s = getUsername();  // fall back to username
    }

    return s;
  };

  static const QString value = get();  // Thread-safe initialization.
  return value;
}

const QString& SystemInfo::getHostname() noexcept {
  auto get = []() {
    QString s = QHostInfo::localHostName().remove('\n').remove('\r').trimmed();

    if (s.isEmpty()) {
      qWarning() << "Could not determine the system's hostname!";
    }

    return s;
  };

  static const QString value = get();  // Thread-safe initialization.
  return value;
}

QString SystemInfo::detectRuntime() noexcept {
  // Manually specified runtime has priority.
  static QString envRuntime = qgetenv("LIBREPCB_RUNTIME").trimmed();
  if (!envRuntime.isEmpty()) {
    return envRuntime;
  }

  // Combine any other autodetected runtime, just in case multiple are set.
  QStringList runtimes;
  static QString envSnap = qgetenv("SNAP").trimmed();
  if (!envSnap.isEmpty()) {
    runtimes << "Snap";
  }
  static QString envFlatpak = qgetenv("FLATPAK_ID").trimmed();
  if (!envFlatpak.isEmpty()) {
    runtimes << "Flatpak";
  }
  static QString envAppimage = qgetenv("APPIMAGE").trimmed();
  if (!envAppimage.isEmpty()) {
    runtimes << "AppImage";
  }
  return runtimes.join(", ");
}

bool SystemInfo::isProcessRunning(qint64 pid) {
#if defined(Q_OS_UNIX)  // Mac OS X / Linux / UNIX
  // From:
  // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qlockfile_unix.cpp
  errno = 0;
  int ret = ::kill(pid, 0);
  if (ret == 0) {
    return true;
  } else if ((ret == -1) &&
             (errno == static_cast<int>(std::errc::no_such_process))) {
    return false;
  } else {
    qDebug() << "errno:" << errno;
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not determine if another process is running."));
  }
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
  // From:
  // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qlockfile_win.cpp
  HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (handle) {
    DWORD exitCode = 0;
    BOOL success = ::GetExitCodeProcess(handle, &exitCode);
    ::CloseHandle(handle);
    if ((success) && (exitCode == STILL_ACTIVE)) {
      return true;
    } else if (success) {
      return false;
    } else {
      qDebug() << "GetLastError():" << GetLastError();
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Could not determine if another process is running."));
    }
  } else if (GetLastError() == ERROR_INVALID_PARAMETER) {
    return false;
  } else {
    qDebug() << "GetLastError():" << GetLastError();
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not determine if another process is running."));
  }
#else
#error "Unknown operating system!"
#endif
}

QString SystemInfo::getProcessNameByPid(qint64 pid) {
  QString processName;
#if defined(Q_OS_OSX)  // macOS
  // From:
  // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qlockfile_unix.cpp
  errno = 0;
  char name[1024] = {0};
  int retval = proc_name(pid, name, sizeof(name) / sizeof(char));
  if (retval > 0) {
    processName = QFile::decodeName(name);
  } else if ((retval == 0) &&
             (errno == static_cast<int>(std::errc::no_such_process))) {
    return QString();  // process not running
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("proc_name() failed with error %1.").arg(errno));
  }
#elif defined(Q_OS_FREEBSD)
  char exePath[64];
  char buf[PATH_MAX + 1];
  sprintf(exePath, "/proc/%lld/file", pid);
  size_t len = (size_t)readlink(exePath, buf, sizeof(buf));
  if (len >= sizeof(buf)) {
    return QString();  // process not running
  }
  buf[len] = 0;
  processName = QFileInfo(QFile::decodeName(buf)).fileName();
#elif defined(Q_OS_OPENBSD)
  // https://man.openbsd.org/sysctl.2
  // NOTE: This will return only the first 16 bytes of the process name. If
  // someone finds a way to get the full process name, feel free to improve it.
  kinfo_proc proc;
  size_t procSize = sizeof(proc);
  int mib[6] = {CTL_KERN,     KERN_PROC, KERN_PROC_PID, static_cast<int>(pid),
                sizeof(proc), 1};
  int retval = sysctl(mib, 6, &proc, &procSize, NULL, 0);
  if (retval != 0) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("sysctl() failed with retval=%1 and errno=%2.")
                           .arg(retval)
                           .arg(errno));
  }
  if (procSize < sizeof(proc)) {
    return QString();  // process not running
  }
  processName = QString::fromLocal8Bit(proc.p_comm, sizeof(proc.p_comm))
                    .section('\0', 0, 0);
#elif defined(Q_OS_LINUX)

  // From:
  // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qlockfile_unix.cpp
  if (!FilePath("/proc/version").isExistingFile()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Could not find the file \"/proc/version\"."));
  }
  char exePath[64];
  char buf[PATH_MAX + 1];
  sprintf(exePath, "/proc/%lld/exe", pid);
  size_t len = (size_t)readlink(exePath, buf, sizeof(buf));
  if (len >= sizeof(buf)) {
    return QString();  // process not running
  }
  buf[len] = 0;
  processName = QFileInfo(QFile::decodeName(buf)).fileName();
  // If the executable does no longer exist, the string " (deleted)" is added to
  // the end of the symlink, so we need to remove that to get the naked process
  // name.
  if (processName.endsWith(" (deleted)"))
    processName.chop(strlen(" (deleted)"));
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
  // Originally from:
  // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qlockfile_win.cpp
  // But then saw this article:
  // https://blogs.msdn.microsoft.com/oldnewthing/20150716-00/?p=45131/ And
  // therefore switched from GetModuleFileNameExW() to
  // QueryFullProcessImageNameW()
  HANDLE hProcess = OpenProcess(
      PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, DWORD(pid));
  if ((!hProcess) && (GetLastError() == ERROR_INVALID_PARAMETER)) {
    return QString();  // process not running
  } else if (!hProcess) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("OpenProcess() failed with error %1.").arg(GetLastError()));
  }
  wchar_t buf[MAX_PATH];
  DWORD length = MAX_PATH;
  BOOL success = QueryFullProcessImageNameW(hProcess, 0, buf, &length);
  CloseHandle(hProcess);
  if ((!success) || (!length)) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("QueryFullProcessImageNameW() failed with error %1.")
                           .arg(GetLastError()));
  }
  processName = QString::fromWCharArray(buf, length);
  int i = processName.lastIndexOf(QLatin1Char('\\'));
  if (i >= 0) processName.remove(0, i + 1);
  i = processName.lastIndexOf(QLatin1Char('.'));
  if (i >= 0) processName.truncate(i);
#elif defined(Q_OS_SOLARIS)
  // https://illumos.org/man/3proc/
  // NOTE: This will only return the first PRFNSZ (16) bytes of the process
  // name. If someone finds a way to get the full process name, feel free to
  // improve this.
  psinfo_t psinfo;
  if (proc_get_psinfo(pid, &psinfo) != 0) {
    return QString();  // process not running
  }
  processName = QString::fromLocal8Bit(psinfo.pr_fname);
#else
#error "Unknown operating system!"
#endif

  // check if the process name is not empty
  if (processName.isEmpty()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not determine the process name of another process."));
  }

  return processName;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
