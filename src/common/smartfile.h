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

#ifndef SMARTFILE_H
#define SMARTFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Class SmartFile
 ****************************************************************************************/

/**
 * @brief   The abstract SmartFile class represents a file and provides some useful
 *          methods to work with that file
 *
 * The constructor will try to open or create the file. The destructor will close the file.
 *
 * Features:
 *  - Open files in read-only mode (this class then guarantees that no write operations
 *    are possible to that file)
 *  - Creation of backup files ('~' at the end of the filename)
 *  - Restoring backup files
 *  - Write all changes (including creation and deletion) to the file system with the
 *    single method #save() (like "commit" in database systems)
 *
 * @note See @ref doc_project_save for more details about the backup/restore feature.
 *
 * @author ubruhin
 * @date 2015-01-25
 *
 * @todo Test the class #SmartFile and all of its subclasses!
 */
class SmartFile : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open or create a file
         *
         * This constructor tries to open an existing or create a new file and throws an
         * exception if an error occurs.
         *
         * @param filepath  The filepath to the (always to a original file, not to a
         *                  backup file with "~" at the end of the filename!)
         * @param restore   If true and a backup (*~) of the specified file exists, the
         *                  backup will be opened instead of the original file. If no
         *                  backup exists or this parameter is false, the original file
         *                  will be opened.
         * @param readOnly  If true, the file will be opened read-only (see #mIsReadOnly)
         * @param create    If true, the file will be created/overwritten after calling
         *                  #save() the first time.
         *
         * @throw Exception If the specified text file could not be opened successful, an
         *                  exception will be thrown.
         */
        SmartFile(const FilePath& filepath, bool restore = false, bool readOnly = false,
                  bool create = false) throw (Exception);

        /**
         * @brief The destructor
         *
         * Removes the temporary file (if existing)
         */
        virtual ~SmartFile() noexcept;


        // Getters

        /**
         * @brief Get the filepath to the file which was passed to the constructor
         *
         * @return The filepath to the file
         */
        const FilePath& getFilepath() const noexcept {return mFilePath;}

        /**
         * @brief Get the "remove flag" (if true, the file will be removed after calling #save())
         *
         * @return The remove flag
         */
        bool getRemoveFlag() const noexcept {return mRemoveFlag;}


        // Setters

        /**
         * @brief Set the "remove flag" (if true, the file will be removed after calling #save())
         *
         * @param removeFlag    if true, the file will be removed after calling #save()
         */
        void setRemoveFlag(bool removeFlag) noexcept {mRemoveFlag = removeFlag;}


        // General Methods

        /**
         * @brief Save the content to the file
         *
         * @param toOriginal    If true, the content will be written to the original file.
         *                      If false, the content will be written to the backup file (*~).
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        void save(bool toOriginal) throw (Exception);


    private:

        // make some methods inaccessible...
        SmartFile();
        SmartFile(const SmartFile& other);
        SmartFile& operator=(const SmartFile& rhs);


    protected:

        // Protected Methods

        /**
         * @brief Pure virtual method which must be implemented in subclasses of #SmartFile
         *
         * This method is called automatically from SmartFile#save() when the content of
         * the file should be saved to the file. So all derived classes must write their
         * content to the file specified by the parameter "filepath". No other actions or
         * checks are needed, simply write the content to this file and throw an exception
         * if writing to the file has failed.
         *
         * @param filepath  The filepath to either the original or the temporary file
         *
         * @throw Exception If writing to the file has failed, an exception will be thrown.
         */
        virtual void saveToFile(const FilePath& filepath) throw (Exception) = 0;


        // Static Protected Methods

        /**
         * @brief Helper method to read the content from a file into a QByteArray
         *
         * @param filepath  The path to the file
         *
         * @return The file content
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        static QByteArray readContentFromFile(const FilePath& filepath) throw (Exception);

        /**
         * @brief Helper method to save the content of a QByteArray to a file
         *
         * This method can be used in derived classes of #SmartFile to simply write a
         * QByteArray to a file. Especially to implement the pure virtual method
         * #saveToFile() this may be useful.
         *
         * @param filepath      The path to the file
         * @param content       The content which will be written to the file
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        static void saveContentToFile(const FilePath& filepath, const QByteArray& content) throw (Exception);


        // General Attributes

        /**
         * @brief The filepath which was passed to the constructor
         */
        FilePath mFilePath;

        /**
         * @brief The filepath to the temporary file (#mFilePath + '~')
         */
        FilePath mTmpFilePath;

        /**
         * @brief The filepath from where the content was loaded
         *
         * If the backup file was loaded, this equals to #mFilePath with appended tilde ('~').
         * If the original file was loaded, this equals to #mFilePath.
         */
        FilePath mOpenedFilePath;

        /**
         * @brief This variable determines whether the file was restored or not
         *
         * This file is set to true when the constructor was called with the parameter
         * "restore == true". After calling #save() with parameter "toOriginal == true",
         * this flag will be reset to false. The destructor needs this flag to decide
         * whether the temporary file should be removed or not.
         */
        bool mIsRestored;

        /**
         * @brief If true, the file is opened as read-only
         *
         * @li No temporary files will be created/removed
         * @li It's not possible to #save() the file (exception will be thrown instead)
         */
        bool mIsReadOnly;

        /**
         * @brief If true, the file was created and not yet written to the filesystem (so
         *        the file #mFilePath does not yet exist!)
         */
        bool mIsCreated;

        /**
         * @brief If true, the file will be removed after calling #save()
         */
        bool mRemoveFlag;
};

#endif // SMARTFILE_H
