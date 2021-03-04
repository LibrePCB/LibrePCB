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

#ifndef LIBREPCB_DIRECTORYLOCK_H
#define LIBREPCB_DIRECTORYLOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "filepath.h"

#include <QtCore>

#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class DirectoryLock
 ******************************************************************************/

/**
 * @brief This class can be used to implement file-based directory locks
 *
 * Many classes of this project open some directories (workspaces, projects,
 * library elements, ...). But it's very dangerous if a directory is opened
 * multiple times simultaneously (by the same or another instance of the
 * application, maybe even on different computers if the directories are located
 * on a network drive). To avoid such problems, this class provides a mechanism
 * to create directory locks.
 *
 *
 * <b>How such a directory lock works:</b>
 *
 * Let's say that you want to open the directory "/foo/bar/". Then a lock file
 * with the filepath "/foo/bar/.lock" will be created. After closing the
 * directory, the lock file will be removed. So, while the directory (e.g. a
 * project) is open, there will be a lock file in the same directory. If the
 * same or another instance of the application now wants to open the same
 * directory at the same time, the lock file is detected and opening the
 * directory will be denied.
 *
 * The lock file is a simple UTF-8 encoded text file with 5 lines with following
 * values:
 *  -# The full name (first name + last name) of the user which holds the lock
 *  -# The username (logon name) of the user which holds the lock
 *  -# The hostname of the user's computer which holds the lock
 *  -# The process id (PID) of the application instance which holds the lock
 *  -# The process name of the application instance which holds the lock
 *  -# The datetime when the lock file was created/updated (UTC and ISO format!)
 *
 * Example:
 * @code
 * Homer Simpson
 * homer
 * homer-workstation
 * 1234
 * librepcb
 * 2013-04-13T12:43:52Z
 * @endcode
 *
 * The lock file (and especially its content) is also used to detect application
 * crashes. If the application crashes while a directory was locked, the lock
 * file will still exist after the application crashed. Now, if the user
 * tries to open the locked directory again, the content of the lock file will
 * be parsed. If the username and the hostname in the lock file is equal to the
 * current user which tries to get the lock, it's clear that the lock file does
 * NOT exist because the locked directory is already open, but that the
 * application crashed while the directory was locked. If there exists a
 * backup of the locked directory (e.g. project auto-save), this allows to ask
 * the user whether the backup should be restored or not.
 *
 *
 * <b>How to use this class:</b>
 *
 * First, you need to create an instance of this class for the directory you
 * want to protect with a lock. There are two different constructors for this
 * purpose. If you use the default constructor, you need to call #setDirToLock()
 * afterwards. Now you can read the lock status of the specified directory with
 * #getStatus(). With #lock() you can create the lock file, and with #unlock()
 * you can remove the lock file. There are also the two convenience methods
 * #tryLock() and #unlockIfLocked(), just read their documentation for more
 * information.
 *
 * @note    The destructor will automatically call #unlockIfLocked().
 *          This allows a reliable implementation of a directory lock, because
 * you can add a DirectoryLock instance to the attributes of your class which
 * access a directory which should be locked. This will ensure that the lock
 * will be released when your object gets destroyed (RAII). See the code example
 * below.
 *
 * Code Example:
 * @code
 * // a class which opens a directory and needs to lock it
 * class MyDirectoryOpeningClass {
 * public:
 *   MyDirectoryOpeningClass()  // constructor
 *     : mLock(FilePath("C:/myDirectory")) { // variant 1 to set the filepath
 *     mLock.setDirToLock(FilePath("C:/myDirectory"));  // variant 2
 *     switch (mLock.getStatus()) { // Note: this line can throw an exception!
 *       case DirectoryLock::LockStatus::Unlocked:
 *         // No lock exists --> lock the directory now
 *         mLock.lock();  // Note: this line can throw an exception!
 *         break;
 *       case DirectoryLock::LockStatus::LockedByThisApp:
 *       case DirectoryLock::LockStatus::LockedByOtherApp:
 *       case DirectoryLock::LockStatus::LockedByOtherUser:
 *       case DirectoryLock::LockStatus::LockedByUnknownApp:
 *         // The directory is (probably) already locked by another instance!
 *         throw Exception("Directory is locked!");
 *       case DirectoryLock::LockStatus::StaleLock:
 *         // The application crashed while the lock was active.
 *         // Ask the user whether a backup should be restored or not.
 *         break;
 *       default:
 *         // Should not happen...
 *         break;
 *     }
 *     // if you don't care about stale locks, you could just do this instead:
 *     // myLock.tryLock(); // Note: this line can throw an exception!
 *   }
 *
 *   ~MyDirectoryOpeningClass() {
 *     // You do not have to (but you could) call myLock.unlockIfLocked(),
 *     // as it will be called automatically in the destructor of myLock.
 *     // try { myLock.unlockIfLocked(); } catch (...) { }
 *   }
 *
 * private:
 *   // an instance, not only a pointer (important for RAII)!
 *   DirectoryLock mLock;
 * };
 * @endcode
 */
class DirectoryLock final {
  Q_DECLARE_TR_FUNCTIONS(DirectoryLock)

public:
  // Types

  /**
   * @brief The return type of #getStatus()
   */
  enum class LockStatus {
    /// The directory is not locked (lock file does not exist).
    Unlocked,
    /// The directory is locked by a crashed application instance.
    StaleLock,
    /// The directory is locked by this application instance.
    LockedByThisApp,
    /// The directory is locked by another application instance on this machine.
    LockedByOtherApp,
    /// The directory is locked by another user or machine.
    LockedByOtherUser,
    /// The directory is locked by an unknown application (may be stale).
    LockedByUnknownApp,
  };

  /**
   * @brief Callback type used to determine whether a lock should be overridden
   *        or not
   *
   * @param dir     The directory to be locked.
   * @param status  The current status of the lock (see #getStatus()).
   * @param user    Name of the user which currently holds the lock.
   *
   * @retval true   Override lock.
   * @retval false  Do not override lock.
   *
   * @throw ::librepcb::UserCanceled to abort locking the directory.
   */
  typedef std::function<bool(const FilePath& dir, LockStatus status,
                             const QString& user)>
      LockHandlerCallback;

  // Constructors / Destructor

  /**
   * @brief The default constructor
   *
   * @warning     If you use this constructor, you need to call #setDirToLock()
   *              afterwards (before calling any other method of this class)!
   */
  DirectoryLock() noexcept;

  /**
   * @brief Copy constructor
   *
   * @param other     The object to copy
   */
  DirectoryLock(const DirectoryLock& other) = delete;

  /**
   * @brief A constructor which will call #setDirToLock()
   *
   * @param dir       See #setDirToLock()
   */
  explicit DirectoryLock(const FilePath& dir) noexcept;

  /**
   * @brief The destructor (this may also unlock the locked file)
   *
   * @note    The destructor will also try to unlock the directory if it was
   * locked with this object.
   */
  ~DirectoryLock() noexcept;

  // Setters

  /**
   * @brief Specify the directory for which you need the lock
   *
   * @param dir      The filepath to the directory to lock
   *
   * @warning This method must not be called when this object already holds a
   * lock!
   */
  void setDirToLock(const FilePath& dir) noexcept;

  // Getters

  /**
   * @brief Get the filepath of the directory to lock (passed by
   * #setDirToLock())
   *
   * @return The filepath to the directory to lock (invalid if no filepath was
   * set)
   */
  const FilePath& getDirToLock() const noexcept { return mDirToLock; }

  /**
   * @brief Get the filepath of the lock file (NOT the directory to lock!)
   *
   * @return The filepath to the lock file (invalid if no valid filepath was
   * set)
   */
  const FilePath& getLockFilepath() const noexcept { return mLockFilePath; }

  /**
   * @brief Get the lock status of the specified directory
   *
   * @param lockedByUser  If not nullptr and the directory is locked, the
   *                      username of the current lock is written into this
   *                      string.
   * @return  The current lock status (see #LockStatus)
   *
   * @throw   Exception on error (e.g. invalid filepath, no access rights, ...)
   */
  LockStatus getStatus(QString* lockedByUser = nullptr) const;

  // General Methods

  /**
   * @brief Lock the specified directory if not already locked
   *
   * This is a save method to get a lock without the need for first reading the
   * lock status with #getStatus(). Depending on the lock status, this method
   * does following:
   * - Unlocked:  Set "wasStale = false" and get the lock (calling #lock())
   * - StaleLock: Set "wasStale = true" and get the lock (calling #lock())
   * - Locked:    Throw exception (something like "Directory already locked")
   *
   *@param lockHandler  If supplied and the directory is already locked, this
   *                    callback gets called to determine whether the lock
   *                    should be overridden or not. If not supplied and the
   *                    directory is locked, an exception will be thrown.
   *
   * @throw   Exception on error (e.g. already locked, no access rights, ...)
   */
  void tryLock(LockHandlerCallback lockHandler = nullptr);

  /**
   * @brief Unlock the specified directory if it was locked by this object
   *
   * If the specified directory is locked by this object, this method calls
   * #unlock(). Otherwise this method does nothing.
   *
   * @return  True if the lock has been released by this object, false
   * otherwise.
   *
   * @throw   Exception on error (e.g. invalid filepath, no access rights, ...)
   */
  bool unlockIfLocked();

  /**
   * @brief Lock the specified directory (create/update the lock file)
   *
   * @warning This method will always overwrite an already existing lock file,
   * even if it was created by another application instance! So: Always check
   *          the lock status first with #getStatus(), or use #tryLock()
   * instead!
   *
   * @throw   Exception on error (e.g. invalid filepath, no access rights, ...)
   */
  void lock();

  /**
   * @brief Unlock the specified directory (remove the lock file)
   *
   * @warning This method will always remove an existing lock file, even if it
   * was created by another application instance! So: Always check the lock
   *          status first with #getStatus(), or use #unlockIfLocked() instead!
   *
   * @throw   Exception on error (e.g. invalid filepath, no access rights, ...)
   */
  void unlock();

  // Operator Overloadings
  DirectoryLock& operator=(const DirectoryLock& rhs) = delete;

private:  // Methods
  /**
   * @brief Get the global set of filepaths locked by this application instance
   *
   * @return Set of directory filepaths currently locked by this instance.
   */
  static QSet<FilePath>& dirsLockedByThisAppInstance() noexcept;

private:  // Data
  /**
   * @brief The filepath to the directory to lock (passed by #setDirToLock())
   */
  FilePath mDirToLock;

  /**
   * @brief The filepath to the lock file
   *
   * Example: If the filepath "/foo/bar" was passed to #setDirToLock(),
   *          this attribute will have the value "/foo/bar/.lock".
   */
  FilePath mLockFilePath;

  /**
   * @brief This attribute defines if the lock is active by this object
   *
   * If #lock() was called successfully, mLockedByThisObject is set to true.
   * If #unlock() was called successfully, mLockedByThisObject is set to false.
   *
   * In other words: This attribute is true while this object has the ownership
   * over the lock file (between calling #lock() and #unlock()).
   *
   * The only goal of this attribute is to decide whether the destructor should
   * remove the lock or not. If the destructor is called while this attribute is
   * true, the destructor will call #unlock() to remove the file lock.
   */
  bool mLockedByThisObject;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_DIRECTORYLOCK_H
