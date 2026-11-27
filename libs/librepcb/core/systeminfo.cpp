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

#include "fileio/filepath.h"

#include <QHostInfo>
#include <QtCore>

#if defined(Q_OS_MACOS)  // macOS
#include <sys/types.h>
#include <system_error>

#include <cerrno>
#include <libproc.h>
#include <signal.h>
#elif defined(Q_OS_UNIX)  // UNIX/Linux
#include <sys/types.h>
#include <system_error>

#include <cerrno>
#include <csignal>
#include <pwd.h>
#include <unistd.h>
#if defined(Q_OS_SOLARIS)
#include <libproc.h>
#endif
#if defined(Q_OS_OPENBSD) || defined(Q_OS_FREEBSD)
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
// nosort
#include <lm.h>
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
  // Thread-safe static initialization.
  static const QString sValue = []() {
    // This line should work for most UNIX, Linux, Mac and Windows systems.
    auto s = QString(qgetenv("USERNAME")).remove('\n').remove('\r').trimmed();
    // If the environment variable "USERNAME" is not set, we will try "USER".
    if (s.isEmpty()) {
      s = QString(qgetenv("USER")).remove('\n').remove('\r').trimmed();
    }
    if (s.isEmpty()) {
      qWarning() << "Could not determine the system's username!";
    }
    return s;
  }();
  return sValue;
}

const QString& SystemInfo::getFullUsername() noexcept {
  // Thread-safe static initialization.
  static const QString sValue = []() {
    QString s;
#if defined(Q_OS_MACOS)  // macOS
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
    if (!userinfo) {
      qWarning() << "Could not fetch user info via getpwuid!";
    } else {
      QString gecosString = QString::fromLocal8Bit(userinfo->pw_gecos);
      s = gecosString.section(',', 0, 0).remove('\n').remove('\r').trimmed();
    }
#elif defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
    // From
    // https://learn.microsoft.com/en-us/windows/win32/api/lmaccess/nf-lmaccess-netusergetinfo.
    LPUSER_INFO_2 buf = NULL;
    const std::wstring username = getUsername().toStdWString();
    const NET_API_STATUS status =
        NetUserGetInfo(NULL, username.data(), 2, (LPBYTE*)&buf);
    if (buf != NULL) {
      if (status == NERR_Success) {
        s = QString::fromWCharArray(buf->usri2_full_name);
      }
      NetApiBufferFree(buf);
      buf = NULL;
    }
    if (s.isEmpty()) {
      // Fallback which generally is known to be working, but only for ASCII
      // names (see https://github.com/LibrePCB/LibrePCB/issues/1365).
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
  }();
  return sValue;
}

const QString& SystemInfo::getHostname() noexcept {
  // Thread-safe static initialization.
  static const QString sValue = []() {
    QString s = QHostInfo::localHostName().remove('\n').remove('\r').trimmed();
    if (s.isEmpty()) {
      qWarning() << "Could not determine the system's hostname!";
    }
    return s;
  }();
  return sValue;
}

QString SystemInfo::detectRuntime() noexcept {
  // Manually specified runtime has priority.
  static QString sEnvRuntime = qgetenv("LIBREPCB_RUNTIME").trimmed();
  if (!sEnvRuntime.isEmpty()) {
    return sEnvRuntime;
  }

  // Combine any other autodetected runtime, just in case multiple are set.
  QStringList runtimes;
  static QString sEnvSnap = qgetenv("SNAP").trimmed();
  if (!sEnvSnap.isEmpty()) {
    runtimes << "Snap";
  }
  static QString sEnvFlatpak = qgetenv("FLATPAK_ID").trimmed();
  if (!sEnvFlatpak.isEmpty()) {
    runtimes << "Flatpak";
  }
  static QString sEnvAppimage = qgetenv("APPIMAGE").trimmed();
  if (!sEnvAppimage.isEmpty()) {
    runtimes << "AppImage";
  }
  return runtimes.join(", ");
}

std::optional<bool> SystemInfo::isProcessRunning(qint64 pid) noexcept {
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
  }
  qCritical().nospace() << "Could not determine if the process with PID " << pid
                        << " is running: kill() failed with errno " << errno
                        << ".";
  return std::nullopt;
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
    }
  } else if (GetLastError() == ERROR_INVALID_PARAMETER) {
    return false;
  }
  qCritical().nospace() << "Could not determine if the process with PID " << pid
                        << " is running: GetLastError() returned "
                        << GetLastError() << ".";
  return std::nullopt;
#else
#error "Unknown operating system!"
#endif
}

std::optional<QString> SystemInfo::getProcessNameByPid(qint64 pid) noexcept {
  QString processName;
#if defined(Q_OS_MACOS)  // macOS
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
    qCritical().nospace() << "Could not determine the process name of PID "
                          << pid << ": proc_name() failed with errno " << errno
                          << ".";
    return std::nullopt;
  }
#elif defined(Q_OS_FREEBSD)
  char pathbuf[PATH_MAX] = {
      0,
  };
  size_t pathbufsz = PATH_MAX;
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME,
                static_cast<pid_t>(pid)};
  int rc = sysctl(mib, 4, pathbuf, &pathbufsz, NULL, 0);
  if (rc == -1) {
    switch (errno) {
      case ESRCH:  // explicitly No such process
        return QString();
      default: {
        int saved_errno = errno;
        char errbuf[512] = {
            0,
        };
        // Should 512 not be enough, the error string returned by strerror_r()
        // will be truncated but still be nul-terminated.
        strerror_r(saved_errno, errbuf, sizeof(errbuf));
        qCritical().nospace()
            << "Could not determine the process name of PID " << pid
            << ": sysctl() failed with errno " << saved_errno << " ("
            << QString::fromLocal8Bit(errbuf) << ").";
        return std::nullopt;
      }
    }
  }
  processName = QFileInfo(QFile::decodeName(pathbuf)).fileName();
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
    qCritical().nospace() << "Could not determine the process name of PID "
                          << pid << ": sysctl() failed with retval " << retval
                          << " and errno " << errno << ".";
    return std::nullopt;
  }
  if (procSize < sizeof(proc)) {
    return QString();  // process not running
  }
  processName = QString::fromLocal8Bit(proc.p_comm, sizeof(proc.p_comm))
                    .section('\0', 0, 0);
#elif defined(Q_OS_LINUX)
  // From:
  // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qlockfile_unix.cpp
  // And added more precise error handling.
  if (!FilePath("/proc/version").isExistingFile()) {
    qCritical().nospace() << "Could not determine the process name of PID "
                          << pid << ": '/proc/version' does not exist.";
    return std::nullopt;
  }
  char exePath[64];
  char buf[PATH_MAX + 1];
  sprintf(exePath, "/proc/%lld/exe", pid);
  ssize_t len = readlink(exePath, buf, sizeof(buf));
  if ((len < 0) &&
      (errno == static_cast<int>(std::errc::no_such_file_or_directory))) {
    return QString();  // process not running
  }
  if ((len < 0) || (static_cast<size_t>(len) >= sizeof(buf))) {
    qCritical() << "Could not determine the process name of PID " << pid
                << ": readlink() failed with errno " << errno << ".";
    return std::nullopt;
  }
  buf[len] = 0;
  processName = QFileInfo(QFile::decodeName(buf)).fileName();
  // If the executable does no longer exist, the string " (deleted)" is added to
  // the end of the symlink, so we need to remove that to get the naked process
  // name.
  if (processName.endsWith(" (deleted)")) {
    processName.chop(strlen(" (deleted)"));
  }
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
    qCritical().nospace() << "Could not determine the process name of PID "
                          << pid << ": OpenProcess() failed with error "
                          << GetLastError() << ".";
    return std::nullopt;
  }
  wchar_t buf[MAX_PATH];
  DWORD length = MAX_PATH;
  BOOL success = QueryFullProcessImageNameW(hProcess, 0, buf, &length);
  CloseHandle(hProcess);
  if ((!success) || (!length)) {
    qCritical().nospace() << "Could not determine the process name of PID "
                          << pid
                          << ": QueryFullProcessImageNameW() failed with error "
                          << GetLastError() << ".";
    return std::nullopt;
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

  // Check if the process name is not empty.
  if (processName.isEmpty()) {
    qCritical().nospace() << "Could not determine the process name of PID "
                          << pid << ".";
    return std::nullopt;
  }

  return processName;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
