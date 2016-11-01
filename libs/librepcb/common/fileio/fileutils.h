/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_FILEUTILS_H
#define LIBREPCB_FILEUTILS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../exceptions.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class FilePath;

/*****************************************************************************************
 *  Class FileUtils
 ****************************************************************************************/

/**
 * @brief The FileUtils class provides some static methods to execute file operations.
 *
 * @author ubruhin
 * @date 2016-08-07
 */
class FileUtils final
{
        Q_DECLARE_TR_FUNCTIONS(FileUtils)

    public:

        // Constructors / Destructor
        FileUtils() = delete;
        FileUtils(const FileUtils& other) = delete;


        // Static methods

        /**
         * @brief Read the content of a file into a QByteArray
         *
         * @param filepath      The file to read
         *
         * @return              The content of the file
         *
         * @throws Exception    If an error occurs.
         */
        static QByteArray readFile(const FilePath& filepath) throw (Exception);

        /**
         * @brief Write the content of a QByteArray into a file
         *
         * If the file does not exist, it will be created (with all parent directories).
         *
         * @param filepath      The file to (over)write
         * @param content       The content to write
         *
         * @throws Exception    If an error occurs.
         */
        static void writeFile(const FilePath& filepath, const QByteArray& content) throw (Exception);

        /**
         * @brief Copy a single file
         *
         * @param source        Filepath to an existing file.
         * @param dest          Filepath to a non-existing file (if it exists already,
         *                      an exception will be thrown).
         *
         * @throws Exception    If an error occurs.
         */
        static void copyFile(const FilePath& source, const FilePath& dest) throw (Exception);

        /**
         * @brief Copy a directory recursively
         *
         * @param source        Filepath to an existing directory.
         * @param dest          Filepath to a non-existing directory (if it exists
         *                      already, an exception will be thrown).
         *
         * @throws Exception    If an error occurs.
         */
        static void copyDirRecursively(const FilePath& source, const FilePath& dest) throw (Exception);

        /**
         * @brief Move/rename a file or directory
         *
         * @param source        Filepath to an existing file or directory.
         * @param dest          Filepath to a non-existing file/directory (if it exists
         *                      already, an exception will be thrown).
         *
         * @throws Exception    If an error occurs.
         */
        static void move(const FilePath& source, const FilePath& dest) throw (Exception);

        /**
         * @brief Remove a single file
         *
         * @param file          Filepath to a file (may or may not exist).
         *
         * @throws Exception    If an error occurs.
         */
        static void removeFile(const FilePath& file) throw (Exception);

        /**
         * @brief Remove a directory recursively
         *
         * @param dir           Filepath to a directory (may or may not exist).
         *
         * @throws Exception    If an error occurs.
         */
        static void removeDirRecursively(const FilePath& dir) throw (Exception);

        /**
         * @brief Create a directory with all parent directories
         *
         * @param path          Filepath to a directory (may or may not exist).
         */
        static void makePath(const FilePath& path) throw (Exception);


        // Operator Overloadings
        FileUtils& operator=(const FileUtils& rhs) = delete;
};

} // namespace librepcb

#endif // LIBREPCB_FILEUTILS_H
