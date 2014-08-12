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

#ifndef FILELOCK_H
#define FILELOCK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "filepath.h"

/*****************************************************************************************
 *  Class FileLock
 ****************************************************************************************/

/**
 * @brief This class can be used to implement file-based file locks
 *
 * Many classes of this project open some files (project files, library files, ...).
 * But it's very dangerous if a file is opened multiple times simultaneously (by the
 * same or another instance of the application, maybe even on different computers if
 * the files are located in a shared folder). To avoid such problems, this class provides
 * a mechanism to create file locks.
 *
 *
 * <b>How such a file lock works:</b>
 *
 * Let's say that you want to open the file "/foo/goo.xml". Then a lock file with the
 * filename "/foo/.~lock.goo.xml#" will be created. After closing your file, the lock file
 * will be removed. So, while the file is open, there will be a lock file in the same
 * directory. If the same or another instance of the application now wants to open the same
 * file at the same time, the lock file is detected and opening the file will be denied.
 *
 * The lock file is a simple UTF-8 coded text file with a comma-seperated list of these
 * values (all in one line, commas in the usernames and hostname are removed):
 *  -# The full name (first name + last name) of the user which holds the lock
 *  -# The username (logon name) of the user which holds the lock
 *  -# The hostname of the user's computer which holds the lock
 *  -# The process id (PID) of the application instance which holds the lock
 *  -# The datetime when the lock file was created/updated (UTC and ISO format!)
 *
 * Example: @code Homer Simpson,homer,homer-workstation,1234,2013-04-13T12:43:52Z @endcode
 *
 * The lock file (and especially its content) is also used to detect application crashes.
 * If the application crashes while a file was locked, the lock file will still exist
 * after the application was closed accidently. Now, if the user tries to open the locked
 * file again, the content of the lock file will be parsed. If the username and the
 * hostname in the lock file is equal to the current user which tries to get the lock,
 * it's clear that the lock file does NOT exist because the locked file is already open,
 * but that the application was crashed while the file was locked. If there exists a
 * backup of the locked file, this allows to ask the user whether the backup should be
 * restored or not. Or alternatively, the lock file can be simply overwritten.
 *
 *
 * <b>How to use this class:</b>
 *
 * First, you need to create an instance of this class for each file you want to protect
 * with a lock. There are two different constructors for this purpose. If you use the
 * default constructor, you need to call #setFileToLock() afterwards. Now you can read
 * the lock status of the specified file with #getStatus(). With #lock() you can create
 * the lock file, and with #unlock() you can remove the lock file.
 *
 * @note    The destructor will automatically unlock the specified file if you created
 *          the lock file with #lock() and didn't removed the lock with #unlock().
 *          This allows a reliable implementation of a file lock, because you can add
 *          a FileLock instance to the attributes of your class which uses a file that
 *          should be locked. This will ensure that the lock will be released when your
 *          object gets destroyed (like RAII). See the code example below (you do not
 *          need to use the FileLock class this way - you can use this class also
 *          without using RAII [Resource Acquisition Is Initialization]).
 *
 * Code Example:
 * @code
 *  class MyFileOpeningClass // a class which opens a file and needs a lock for it
 *  {
 *      public:
 *          MyFileOpeningClass() // constructor
 *              : myLock(FilePath("C:/myFile.txt")) // variant 1 to set the filepath
 *          {
 *              myLock.setFileToLock(FilePath("C:/myFile.txt")); // variant 2
 *              switch (myLock.getStatus())
 *              {
 *                  case Unlocked:
 *                      myLock.lock(); // lock the file now
 *                      break;
 *                  case Locked:
 *                      // The file is locked by another instance. You cannot open the file.
 *                      break;
 *                  case StaleLock:
 *                      // The application was crashed while the lock was active.
 *                      // Ask the user whether a backup should be restored or not.
 *                      break;
 *                  default:
 *                      // There was an error with the file lock
 *                      break;
 *              }
 *
 *          }
 *          ~MyFileOpeningClass() // destructor
 *          {
 *              // You do not have to (but you could) call myLock.unlock(), as it will be
 *              // called automatically in the destructor of myLock (only because you have
 *              // called myLock.lock() in the constructor and the lock is still active)!
 *          }
 *
 *      private:
 *          FileLock myLock; // an instance, not only a pointer (important for RAII)!
 *  };
 * @endcode
 *
 * @note    You do not have to create a FileLock object for each file you want to protect.
 *          For example, to lock a whole project with all its (many!) files, one single
 *          lock file for the main project file (*.e4u) is enough. The class
 *          project::Project then will handle the file lock. If the project file is
 *          locked, the whole project will not be opened.
 *
 * @author ubruhin
 * @date 2014-07-29
 */
class FileLock final
{
    public:

        // Types

        /**
         * @brief The return type of #getStatus()
         */
        enum LockStatus {
            Unlocked,   ///< the file is not locked (lock file does not exist)
            Locked,     ///< the file is locked by another application instance
            StaleLock,  ///< the file is locked by a crashed application instance
            Error       ///< an error is occured while determining the lock status
        };


        // Constructors / Destructor

        /**
         * @brief The default constructor
         *
         * @warning     If you use this constructor, you need to call #setFileToLock()
         *              afterwards (before calling any other method of this class)!
         *              Never call another method before the filepath was set!
         */
        explicit FileLock() noexcept;

        /**
         * @brief A constructor which will call #setFileToLock()
         *
         * @note    If you use this constructor instead of the default constructor, you
         *          do not need to call #setFileToLock() afterwards.
         *
         * @param filepath  See #setFileToLock()
         */
        explicit FileLock(const FilePath& filepath) noexcept;

        /**
         * @brief The destructor (this may also unlock the locked file)
         *
         * @note The destructor will also unlock the file if it was locked with this object!
         */
        ~FileLock() noexcept;


        // Setters

        /**
         * @brief Specify the file for which you need the lock (NOT the lock file itself!)
         *
         * @param filepath      The filepath to the file to lock (it do not need to exist)
         *
         * @return true if success, false if not
         */
        bool setFileToLock(const FilePath& filepath) noexcept;


        // Getters

        /**
         * @brief Get the filepath of the lock file (NOT the file passed by setFileToLock()!)
         *
         * @return The filepath to the lock file (invalid if no valid filepath was set)
         */
        const FilePath& getLockFilepath() const noexcept;

        /**
         * @brief Get the lock status of the specified file
         *
         * @return The current lock status (see #LockStatus)
         *
         * @todo    This method cannot detect if a lock file was created by another
         *          application instance on the same computer with the same user.
         *          If multiple instances of the application are not allowed, this
         *          should't be a problem. Otherwise, the PID in the lock file must
         *          be considered (and check if such a process exists).
         */
        LockStatus getStatus() const noexcept;


        // General Methods

        /**
         * @brief Lock the specified file (create/update the lock file)
         *
         * @warning This method will always overwrite an already existing lock file,
         *          even if that lock file was created by another application instance!
         *          So: Always check first the lock status with #getStatus()!
         *
         * @return  True on success, false on failure
         */
        bool lock() noexcept;

        /**
         * @brief Unlock the specified file (remove the lock file)
         *
         * @warning This method will always remove an existing lock file,
         *          even if that lock file was created by another application instance!
         *          So: Always check first the lock status with #getStatus()!
         *
         * @return  True on success, false on failure
         */
        bool unlock() noexcept;


    private:

        // make some methods inaccessible...
        FileLock(const FileLock& other);
        FileLock& operator=(const FileLock& rhs);

        /**
         * @brief The filepath to the lock file
         *
         * Example: If the filepath "C:/foo/goo.xml" was passed to setFileToLock(),
         *          this attribute will have the value "C:/foo/.~lock.goo.xml#".
         */
        FilePath mLockFilepath;

        /**
         * @brief This attribute defines if the lock is active by this object
         *
         * If #lock() was called successfully, mLockedByThisObject is set to true.
         * If #unlock() was called successfully, mLockedByThisObject is set to false.
         *
         * In other words: This attribute is true while this object has the "ownership"
         * over the lock file (between calling #lock() and #unlock()).
         *
         * The only goal of this attrubute is to decide whether the destructor should
         * remove the lock or not. If the destructor is called while this attribute is
         * true, the destructor will call #unlock() to remove the file lock.
         */
        bool mLockedByThisObject;
};

#endif // FILELOCK_H
