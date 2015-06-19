/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef SMARTINIFILE_H
#define SMARTINIFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "smarttextfile.h"

/*****************************************************************************************
 *  Class SmartIniFile
 ****************************************************************************************/

/**
 * @brief The SmartIniFile class represents an INI file and provides access over QSettings objects
 *
 * The constructor will try to open an INI file. Then you can create QSettings object with
 * #createQSettings() to access the INI file. After making changes, you must release the
 * QSettings object with #releaseQSettings(). With SmartFile#save() the settings can be
 * written back to the INI file. The destructor will close the file. With the static method
 * #create() you can create new, empty INI files.
 *
 * @note See class #SmartFile for more information.
 *
 * @author ubruhin
 * @date 2014-08-23
 *
 * @todo    the handling with QSettings objects is a dirty hack...
 *          make this better...anytime ;-)
 */
class SmartIniFile final : public SmartFile
{
        Q_DECLARE_TR_FUNCTIONS(SmartIniFile)

    public:

        // Constructors / Destructor

        /**
         * @copydoc SmartFile#SmartFile()
         */
        SmartIniFile(const FilePath& filepath, bool restore, bool readOnly,
                     int expectedVersion = -1) throw (Exception) :
            SmartIniFile(filepath, restore, readOnly, false, expectedVersion, -1) {}

        /**
         * @copydoc SmartFile#~SmartFile()
         */
        ~SmartIniFile() noexcept;


        // Getters

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
         * @brief Write all changes to the file system
         *
         * @param toOriginal    Specifies whether the original or the backup file should
         *                      be overwritten/created.
         *
         * @throw Exception If an error occurs
         */
        void save(bool toOriginal) throw (Exception);


        // Static Methods

        /**
         * @brief Create a new (empty) INI file
         *
         * If the file does already exist, it will be overwritten.
         *
         * @param filepath  The filepath of the new INI file
         * @param version   The file version, or -1 if no version number should be added
         *
         * @return A pointer to the new SmartIniFile object
         *
         * @throw Exception If an error occurs, this method throws an exception.
         */
        static SmartIniFile* create(const FilePath& filepath, int version = -1) throw (Exception);


    private:

        // make some methods inaccessible...
        SmartIniFile();
        SmartIniFile(const SmartIniFile& other);
        SmartIniFile& operator=(const SmartIniFile& rhs);


        // Private Methods

        /**
         * @copydoc SmartFile#SmartFile()
         */
        SmartIniFile(const FilePath& filepath, bool restore, bool readOnly, bool create,
                     int expectedVersion, int createVersion) throw (Exception);


        // General Attributes

        /**
         * @brief The filepath to the file in the temporary directory
         */
        FilePath mTmpIniFilePath;

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

#endif // SMARTINIFILE_H
