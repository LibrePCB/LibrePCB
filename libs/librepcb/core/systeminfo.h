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

#ifndef LIBREPCB_CORE_SYSTEMINFO_H
#define LIBREPCB_CORE_SYSTEMINFO_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SystemInfo
 ******************************************************************************/

/**
 * @brief This class provides some methods to get information from the operating
 * system
 *
 * For example, this class is used to get the name of the user which is logged
 * in and the hostname of the computer to create a lock file (see class
 * ::librepcb::DirectoryLock).
 *
 * @note Only static methods are available. You cannot create objects of this
 * class.
 */
class SystemInfo final {
  Q_DECLARE_TR_FUNCTIONS(SystemInfo)

public:
  // Constructors/Destructor
  SystemInfo() = delete;

  /**
   * @brief Get the name of the user which is logged in (like "homer")
   *
   * @return The username (in case of an error, this string can be empty!)
   */
  static const QString& getUsername() noexcept;

  /**
   * @brief Get the full name of the user which is logged in (like "Homer
   * Simpson")
   *
   * @return The full user name (can be empty)
   */
  static const QString& getFullUsername() noexcept;

  /**
   * @brief Get the hostname of the computer (like "homer-workstation")
   *
   * @return The hostname (in case of an error, this string can be empty!)
   */
  static const QString& getHostname() noexcept;

  /**
   * @brief Detect the environment in which LibrePCB runs
   *
   * @return Runtime name(s) like "Snap"or "Flatpak", or an empty string.
   */
  static QString detectRuntime() noexcept;

  /**
   * @brief Check whether a process with a given PID is running or not
   *
   * @param pid   The process ID to query
   *
   * @return  True if the process is running, fals if not
   *
   * @throw  Exception    In case of an error.
   */
  static bool isProcessRunning(qint64 pid);

  /**
   * @brief Get the process name of a given PID
   *
   * @param pid   The process ID (may be a running process or not)
   *
   * @return  The name of the given process ID, or an empty string if no process
   *          with the given PID exists.
   *
   * @throw  Exception    In case of an error.
   */
  static QString getProcessNameByPid(qint64 pid);

private:
  // Cached Data
  static QString sUsername;
  static QString sFullUsername;
  static QString sHostname;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
