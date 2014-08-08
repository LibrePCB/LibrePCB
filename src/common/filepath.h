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

#ifndef FILEPATH_H
#define FILEPATH_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"

/*****************************************************************************************
 *  Class FilePath
 ****************************************************************************************/

/**
 * @brief This class represents absolute, well-formatted paths to files or directories
 *
 *
 * <b>Why we need well-formatted paths and what do they look like</b>
 *
 * Using QString for paths to directories or files works not bad, but there are some
 * disadventages. As the receiver of a filepath in form of a QString, you...
 *  - never know whether it uses native separators or not ("/" or "\", depends on the OS)
 *  - never know whether it is an absolute or a relative filepath
 *  - never know whether it contains redundant separators ("//" or "\\")
 *  - never know whether it contains "." or ".." entries
 *  - never know whether a filepath to a directory ends with a separator or not
 *
 * This means, you have to make a lot of checks at runtime to verify the filepath. But
 * that's annoying and can lead into problems if you do not make these checks always solid.
 *
 * The class #FilePath provides a way to avoid such problems and tries to reduce the count
 * of runtime checks you have to perform yourself (the checks will be performed anyway,
 * but the class #FilePath will do this for you).
 *
 * For compatibility reasons between different operating systems, we should follow these
 * rules for all paths in the whole project (if not explicitly needed other formats):
 *  - Always use absolute paths, never relative paths (relative paths can be dangerous!)
 *  - Always use "/" as directory separator (UNIX style), never "\" (Windows style)
 *  - A filepath to a directory must never end with a slash (except the UNIX root "/")
 *  - Never use redundant separators ("//") in paths
 *  - Never use "." and ".." in paths
 *
 * @note For convenience, a filepath which satisfies ALL these rules is called a "well
 *       formatted filepath" in this documentation.
 *
 * To reach this goal, you need to convert each filepath which comes from outside the
 * application (user input, read from file, ...) immediately into such a well-formatted
 * filepath (make absolute, convert separators, remove redundant entries and so on). Then
 * the filepath can be proccessed by the application. If a filepath must be printed to
 * outside the application (print to a message box, write to a file, ...), the filepath
 * can be converted (back) to the needed format (maybe "\" instead of "/", maybe relative
 * instead of absolute, ...).
 *
 *
 * <b>How the class #FilePath works and how to use it</b>
 *
 * The class #FilePath represents a well-formatted filepath to a file or directory and
 * provides methods to convert paths between different formats. The main principe of this
 * class is very simple but powerful: <b>It's not possible to create a #FilePath object of
 * a filepath which is not well-formatted!</b> So every existing #FilePath object
 * represents always "automatically" a well-formatted filepath! This means, if you use
 * #FilePath instead of QStings, you never have to check if a filepath is well-formatted.
 *
 * There is only one little disadvantage of this principe: The constructor will throw an
 * exception if you pass a filepath which cannot be converted to a well-formatted filepath.
 * So you should always keep in mind that you may use a try/catch-block on creating a
 * #FilePath object. Be careful if you use #FilePath objects (by value) as class attributes.
 *
 * <b>Example:</b>
 * @code
 * FilePath fp("C:\foo\bar.txt"); // a file (this line can throw an exception!)
 * qDebug(fp.toStr());            // "C:/foo/bar.txt" <-- this is the well-formatted filepath
 * qDebug(fp.toNative());         // "C:\foo\bar.txt" on Windows, otherwise "C:/foo/bar.txt"
 * fp.setPathEx("/foo/bar/");     // a directory (this line can throw an exception!)
 * qDebug(fp.toStr());            // "/foo/bar" <-- well-formatted (no slash at the end!)
 * qDebug(fp.toNative());         // "\foo\bar" on Windows, otherwise "/foo/bar"
 * @endcode
 *
 * @note    A filepath represented by a FilePath object do not need to exist on the file
 *          system. All methods of this class do not depend on whether a filepath exists
 *          or not, with one exception: The method #toUnique() tries to resolve symbolic
 *          links, which is only possible if the filepath exists. But nevertheless it will
 *          never return invalid or empty paths (the original filepath will be returned
 *          instead). There are also methods to check if the filepath points to an existing
 *          file (#isExistingFile()) or to an existing directory (#isExistingDir()).
 *
 * @warning Please consider that the conversion from filepaths with backslashes as
 *          directory separator (Windows style) to filepaths with slashes as directory
 *          separator (UNIX style, well-formatted paths) will work only on windows! The
 *          reason is that backslashes are allowed in UNIX filepaths (and are NOT
 *          interpreted as directory separator), so it's not allowed to replace all
 *          backslashes in a UNIX filepath with slashes!
 *
 * @warning Be careful with special characters in filepaths! Especially (relative)
 *          filepaths in the library or in projects must always work on all operating
 *          systems (the whole workspace must be platform independent!). This means
 *          that only characters are allowed which can be used in filepaths on all
 *          operating systems (for example the backslash "\" is allowed in UNIX filenames,
 *          but not in Windows filenames). Other filepaths, like filepaths to recently
 *          used projects (which are stored in the user's profile), do not need to be
 *          platform independent.
 *
 * @author ubruhin
 *
 * @date 2014-07-31
 */
class FilePath final
{
    public:

        // Constructors / Destructor

        /**
         * @brief Constructor to create a #FilePath object from a QString
         *
         * The passed filepath will be converted to a well-formatted filepath if possible.
         * Otherwise, this constructor throws an exception.
         *
         * @param filepath  See #setPath()
         *
         * @throw Exception If the conversion fails, the constructor will throw an
         *                  Exception. This is to deny creating objects with
         *                  non-well-formatted paths.
         */
        FilePath(const QString& filepath) throw (Exception);

        /**
         * @brief The copy constructor
         *
         * This constructor will never throw an exception because "other" contains always
         * a well-formatted filepath, so the new object will also contain a well-formatted
         * filepath.
         *
         * @param other     The object to copy
         */
        FilePath(const FilePath& other) noexcept : mPath(other.mPath) {}


        // Setters

        /**
         * @brief Set a new filepath (without throwing an exception)
         *
         * Exactly the same as #setPathEx() except that this method will never throw an
         * exception. Instead, this method has a return value.
         *
         * @param filepath  An absolute (!) filepath to a file or directory (the target do
         *                  not need to exist). On Windows, both forward ("/") and
         *                  backward ("\") slashes are allowed as directory separators
         *                  (also mixed in one filepath). On other operating systems, only
         *                  forward slashes ("/") are allowed!. Also ".", ".." and
         *                  redundant slashes are allowed.
         *
         * @return true on success, false on error (then the old filepath will be kept!)
         *
         * @see #setPathEx()
         */
        bool setPath(const QString& filepath) noexcept;

        /**
         * @brief Set a new filepath (with throwing an exception if the conversion fails)
         *
         * Exactly the same as #setPath() except that this method will throw an exception
         * if the specified filepath cannot be converted to a well-formatted filepath.
         *
         * @param filepath  See #setPath()
         *
         * @throw Exception If the new filepath could not be converted to a well-formatted
         *                  filepath, an Exception will be thrown (and the old filepath
         *                  will be kept!).
         *
         * @see #setPath()
         */
        void setPathEx(const QString& filepath) throw (Exception);


        // Getters

        /**
         * @brief Get the absolute and well-formatted filepath as a QString
         *
         * @return The absolute and well-formatted filepath
         */
        const QString& toStr() const noexcept {return mPath;}

        /**
         * @brief Get the absolute filepath with native directory separators ("/" resp. "\")
         *
         * @return  The same absolute filepath, but with native separators (on other
         *          platforms than Windows, this filepath is identical to the
         *          well-formatted filepath)
         */
        QString toNative() const noexcept {return QDir::toNativeSeparators(mPath);}

        /**
         * @brief Get a unique version of the filepath (resolve symbolic links if possible)
         *
         * Because of symbolic links, the user is able to have different paths which all
         * point to the same file/directory. But sometimes you want to determine whether
         * two paths point to the same file/directory ("equal paths") or not ("different
         * paths"). For this purpose you can use #toUnique(). This method will resolve
         * symbolic links if possible (this is only possible if the filepath exists!).
         * If the filepath does not exist, this method will return the same as #toStr().
         *
         * @return  The filepath with resolved symbolic links (if possible, otherwise the
         *          unchanged filepath which equals to #toStr())
         */
        QString toUnique() const noexcept;

        /**
         * @brief Convert an absolute filepath to a relative filepath (relative to another
         *        filepath)
         *
         * @param base  The base of the relative filepath (the part which will be removed
         *              from the absolute filepath). This must be a filepath to a
         *              directory, paths to a file will produce wrong results!
         *
         * @return A well-formatted filepath with the exception that it is not absolute
         *
         * @note This method is very useful to store relative paths in (text) files.
         *
         * @see #fromRelative()
         */
        QString toRelative(const FilePath& base) const noexcept;

        /**
         * @brief Check if the specified filepath is an existing file
         *
         * @return true if the path points to an existing file, false otherwise
         */
        bool isExistingFile() const noexcept;

        /**
         * @brief Check if the specified filepath is an existing directory
         *
         * @return true if the path points to an existing directory, false otherwise
         */
        bool isExistingDir() const noexcept;


        // Static Methods

        /**
         * @brief Build an absolute and well-formatted filepath from a relative filepath
         *
         * @param base      The base of the relative filepath (the part which is missed
         *                  from the absolute filepath). This must be a filepath to a
         *                  directory, paths to a file will produce wrong results!
         * @param relative  The relative path (relative to "base")
         *
         * @return A #FilePath object with the absolute filepath
         *
         * @throw Exception If there was an error while creating the absolute filepath,
         *                  this method throws an exception.
         *
         * @note This method is very useful to load relative paths from (text) files.
         *
         * @see #toRelative()
         */
        static FilePath fromRelative(const FilePath& base, const QString& relative);


        // Operator Overloadings

        /**
         * @brief The assign operator to copy a FilePath into another FilePath object
         *
         * @warning This method will always success, so it never throws an exception.
         *          But you should take care if you want to assign a QString to a FilePath
         *          object. As there is such a constructor without the keyword "explicit",
         *          this will work, but the implicit called constructor could throw an
         *          exception! So it's possible to write <tt>myPathObj = myQStringObj;</tt>,
         *          but this line can throw an exception.
         */
        FilePath& operator=(const FilePath& rhs) noexcept;


    private:

        // Private Methods

        /**
         * @brief Make the default constructor inaccessible
         */
        FilePath();

        /**
         * @brief Make a filepath well-formatted (except making it absolute!)
         *
         * @param filepath  An absolute or relative filepath which may isn't well-formatted
         *
         * @return  A filepath which satisfies all rules for well-formatted paths, except
         *          that it is absolute (it can be relative!)
         */
        static QString makeWellFormatted(const QString& filepath) noexcept;


        // Attributes

        QString mPath; ///< the absolute and well-formatted filepath
};

#endif // FILEPATH_H
