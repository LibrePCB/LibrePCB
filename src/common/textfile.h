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

#ifndef TEXTFILE_H
#define TEXTFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Class TextFile
 ****************************************************************************************/

/**
 * @brief The TextFile class represents a text file and provides access to its content
 *
 * The constructor will try to open a text file. The destructor will close the file. With
 * the static method #create() you can create new, empty text files.
 *
 * @note    This class is also able to load from and save to backup files ("~" at the end
 *          of the filename). See @ref doc_project_save for more details.
 *
 * @author ubruhin
 * @date 2015-01-19
 */
class TextFile : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open a text file
         *
         * This constructor tries to open an existing text file and throws an exception
         * if an error occurs.
         *
         * @param filepath  The filepath to the text file (always to a original file, not
         *                  to a backup file with "~" at the end of the filename!)
         * @param restore   If true and a backup (*~) of the specified text file exists,
         *                  the backup will be opened instead of the original file. If no
         *                  backup exists or this parameter is false, the original file
         *                  will be opened.
         * @param readOnly  If true, the file will be opened read-only (see #mIsReadOnly)
         *
         * @throw Exception If the specified text file could not be opened successful, an
         *                  exception will be thrown.
         */
        TextFile(const FilePath& filepath, bool restore = false, bool readOnly = false) throw (Exception);

        /**
         * @brief The destructor
         *
         * Removes the temporary file
         */
        virtual ~TextFile() noexcept;


        // Getters

        /**
         * @brief Get the filepath to the text file which was passed to the constructor
         *
         * @return The filepath to the text file
         */
        const FilePath& getFilepath() const noexcept {return mFilePath;}

        /**
         * @brief Get the content of the file
         *
         * @return The content of the file
         */
        const QByteArray& getContent() const noexcept {return mContent;}


        // Setters

        /**
         * @brief Set the content of the text file
         *
         * @note The content won't be written to the file until #save() is called.
         *
         * @param content   The new content of the text file
         */
        virtual void setContent(const QByteArray& content) noexcept {mContent = content;}


        // General Methods

        /**
         * @brief Remove the text file (and its backup file) from the filesystem
         *
         * @throw Exception If the files could not be removed successfully
         *
         * @warning You must not call #save() after calling this method, as this would
         *          re-create the removed files!
         */
        virtual void remove() const throw (Exception);

        /**
         * @brief Save the content back to the text file
         *
         * @param toOriginal    If true, the text will be written to the original file. If
         *                      false, the text will be written to the backup file (*~).
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        virtual void save(bool toOriginal) throw (Exception);


        // Static Methods

        /**
         * @brief Create a new empty text file
         *
         * If the file does already exist, it will be overwritten.
         *
         * @note This method will NOT create the original file specified with "filepath"!
         *       Instead it will only create it's backup file (*.*~). You need to call
         *       #save() afterwards if you also want to create the original file.
         *
         * @param filepath  The filepath of the new text file
         *
         * @return A pointer to the new TextFile object
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        static TextFile* create(const FilePath& filepath) throw (Exception);


    private:

        // make some methods inaccessible...
        TextFile();
        TextFile(const TextFile& other);
        TextFile& operator=(const TextFile& rhs);


    protected:

        // Static Protected Methods

        /**
         * @brief Save the content of a QByteArray to a file
         *
         * If the file does not exist, this method will try to create it (with all parent
         * directories).
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
         * @brief The filepath from where #mContent was loaded
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
         * @li It's not possible to save the file (exception will be thrown instead)
         */
        bool mIsReadOnly;

        /**
         * @brief The content of the text file
         */
        QByteArray mContent;
};

#endif // TEXTFILE_H
