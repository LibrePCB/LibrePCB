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

#ifndef SMARTTEXTFILE_H
#define SMARTTEXTFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "smartfile.h"

/*****************************************************************************************
 *  Class SmartTextFile
 ****************************************************************************************/

/**
 * @brief The SmartTextFile class represents a text file and provides access to its content
 *
 * @note See class #SmartFile for more information.
 *
 * @author ubruhin
 * @date 2015-01-19
 */
class SmartTextFile final : public SmartFile
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open an existing text file
         *
         * This constructor tries to open an existing file and throws an exception if an
         * error occurs.
         *
         * @param filepath  See SmartFile#SmartFile()
         * @param restore   See SmartFile#SmartFile()
         * @param readOnly  See SmartFile#SmartFile()
         *
         * @throw Exception See SmartFile#SmartFile()
         */
        SmartTextFile(const FilePath& filepath, bool restore, bool readOnly) throw (Exception) :
            SmartTextFile(filepath, restore, readOnly, false) {}

        /**
         * @copydoc SmartFile#~SmartFile()
         */
        ~SmartTextFile() noexcept;


        // Getters

        /**
         * @brief Get the content of the file
         *
         * @return The content of the file
         */
        const QByteArray& getContent() const noexcept {return mContent;}


        // Setters

        /**
         * @brief Set the content of the file
         *
         * @note The content won't be written to the file until #save() is called.
         *
         * @param content   The new content of the file
         */
        void setContent(const QByteArray& content) noexcept {mContent = content;}


        // General Methods

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
         * @brief Create a new text file
         *
         * @note    This method will NOT immediately create the file! The file will be
         *          created after calling #save().
         *
         * @param filepath  The filepath to the file to create (always to the original file,
         *                  not to the backup file with "~" at the end of the filename!)
         *
         * @return The #SmartTextFile object of the created file
         *
         * @throw Exception If an error occurs
         */
        static SmartTextFile* create(const FilePath& filepath) throw (Exception);


    private:

        // make some methods inaccessible...
        SmartTextFile();
        SmartTextFile(const SmartTextFile& other);
        SmartTextFile& operator=(const SmartTextFile& rhs);


    protected:

        // Protected Methods

        /**
         * @brief Constructor to create or open a text file
         *
         * @param filepath  See SmartFile#SmartFile()
         * @param restore   See SmartFile#SmartFile()
         * @param readOnly  See SmartFile#SmartFile()
         * @param create    See SmartFile#SmartFile()
         *
         * @throw Exception See SmartFile#SmartFile()
         */
        SmartTextFile(const FilePath& filepath, bool restore, bool readOnly, bool create) throw (Exception);


        // General Attributes

        /**
         * @brief The content of the text file
         */
        QByteArray mContent;
};

#endif // SMARTTEXTFILE_H
