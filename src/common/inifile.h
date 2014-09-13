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

#ifndef INIFILE_H
#define INIFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Class IniFile
 ****************************************************************************************/

/**
 * @brief The IniFile class represents an INI file and provides access over QSettings objects
 *
 * The constructor will try to open an INI file. Then you can create QSettings object with
 * #createQSettings() to access the INI file. After making changes, you must release the
 * QSettings object with #releaseQSettings(). With #save() the settings can be written
 * back to the INI file. The destructor will close the file. With the static method
 * #create() you can create new, empty INI files.
 *
 * @note    This class is also able to load from and save to backup files ("~" at the end
 *          of the filename). See @ref doc_project_save for more details.
 *
 * @author ubruhin
 * @date 2014-08-23
 */
class IniFile final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open a INI file
         *
         * This constructor tries to open an existing INI file and throws an exception
         * if an error occurs.
         *
         * @param filepath  The filepath to the INI file (always to a original file, not
         *                  to a backup file with "~" at the end of the filename!)
         * @param restore   If true and a backup (*~) of the specified INI file exists,
         *                  the backup will be opened instead of the original file. If no
         *                  backup exists or this parameter is false, the original file
         *                  will be opened.
         *
         * @throw Exception If the specified INI file could not be opened successful, an
         *                  exception will be thrown.
         */
        IniFile(const FilePath& filepath, bool restore) throw (Exception);

        /**
         * @brief The destructor
         *
         * Removes all temporary files
         */
        ~IniFile() noexcept;


        // Getters

        /**
         * @brief Get the filepath to the INI file which was passed to the constructor
         *
         * @return The filepath to the INI file
         */
        const FilePath& getFilepath() const noexcept {return mFilepath;}

        /**
         * @brief Get the version of the file (the value of the key "meta/file_version")
         *
         * @return The file version (or -1 if no version is defined in the file)
         */
        int getFileVersion() const noexcept {return mFileVersion;}


        // Setters

        /**
         * @brief Set the version of the file (the value of the key "meta/file_version")
         *
         * @param version   The new version number
         *
         * @throw Exception on error
         */
        void setFileVersion(int version) throw (Exception);


        // General Methods

        /**
         * @brief Create a new QSettings object to access the INI file
         *
         * @return A pointer to the new QSettings object
         *
         * @throw Exception on error
         */
        QSettings* createQSettings() throw (Exception);

        /**
         * @brief Release (delete) a QSettings which was created with #createQSettings()
         *
         * @param settings  A pointer to the QSettings object to release/delete
         */
        void releaseQSettings(QSettings* settings) noexcept;

        /**
         * @brief Remove the INI file (and its backup files) from the filesystem
         *
         * @throw Exception If the files could not be removed successfully
         *
         * @warning You must not call #save() after calling this method, as this would
         *          re-create the removed files! You also should have released all used
         *          QSettings object, otherwise the file in the temporary directory will
         *          not be removed.
         */
        void remove() const throw (Exception);

        /**
         * @brief Save the INI file to the filesystem
         *
         * @param toOriginal    If true, the content will be written to the original file.
         *                      If false, the INI will be written to the backup file (*~).
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        void save(bool toOriginal) throw (Exception);


        // Static Methods

        /**
         * @brief Create a new (empty) INI file
         *
         * If the file does already exist, it will be overwritten.
         *
         * @note This method will NOT create the original file specified with "filepath"!
         *       Instead it will only create it's backup file (*.ini~). You need to call
         *       #save() afterwards if you also want to create the original file.
         *
         * @param filepath  The filepath of the new INI file
         * @param version   The file version, or -1 if no version number should be added
         *
         * @return A pointer to the new IniFile object
         *
         * @throw Exception If an error occurs, this method throws an exception.
         */
        static IniFile* create(const FilePath& filepath, int version = -1) throw (Exception);


    private:

        // make some methods inaccessible...
        IniFile();
        IniFile(const IniFile& other);
        IniFile& operator=(const IniFile& rhs);


        // General Attributes

        /**
         * @brief The filepath which was passed to the constructor
         */
        FilePath mFilepath;

        /**
         * @brief The filepath to the file in the temporary directory
         */
        FilePath mTmpFilepath;

        /**
         * @brief QSettings objects to access the INI file
         */
        QList<QSettings*> mSettings;

        /**
         * @brief The file version (attribute of key "meta/file_version")
         *
         * If the file does not contain the version number, this attribute is -1.
         */
        int mFileVersion;
};

#endif // INIFILE_H
