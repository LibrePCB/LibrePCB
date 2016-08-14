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

#ifndef LIBREPCB_FILEPATH_H
#define LIBREPCB_FILEPATH_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

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
 * provides methods to convert paths between different formats. Every #FilePath object
 * represents either a well-formatted filepath or an invalid object (see #isValid()). It's
 * not possible to create #FilePath objects with non-well-formatted filepaths.
 *
 * <b>Example:</b>
 * @code
 * FilePath fp("C:\foo\bar.txt"); // a file
 * qDebug(fp.toStr());            // "C:/foo/bar.txt" <-- this is the well-formatted filepath
 * qDebug(fp.toNative());         // "C:\foo\bar.txt" on Windows, otherwise "C:/foo/bar.txt"
 * fp.setPath("/foo/bar/");       // a directory
 * qDebug(fp.toStr());            // "/foo/bar" <-- well-formatted (no slash at the end!)
 * qDebug(fp.toNative());         // "\foo\bar" on Windows, otherwise "/foo/bar"
 * @endcode
 *
 * @note    A filepath represented by a FilePath object do not need to exist on the file
 *          system. Most methods of this class do not depend on whether a filepath exists
 *          or not. Exceptions: #toUnique(), #isExistingFile(), #isExistingDir().
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
 * @date 2014-07-31
 */
class FilePath final
{
    public: // Types

        enum CleanFileNameOption {
            // spaces
            KeepSpaces      = 0<<1,
            ReplaceSpaces   = 1<<1,
            // case
            KeepCase        = 0<<2,
            ToLowerCase     = 1<<2,
            // default
            Default         = KeepSpaces | KeepCase,
        };
        Q_DECLARE_FLAGS(CleanFileNameOptions, CleanFileNameOption);


    public: // Methods

        // Constructors / Destructor

        /**
         * @brief The default constructor (this will create an invalid object!)
         */
        FilePath() noexcept;

        /**
         * @brief Constructor to create a #FilePath object from a QString
         *
         * @param filepath      See #setPath()
         */
        explicit FilePath(const QString& filepath) noexcept;

        /**
         * @brief The copy constructor
         *
         * @param other         The object to copy
         */
        FilePath(const FilePath& other) noexcept;


        // Setters

        /**
         * @brief Set a new filepath
         *
         * @param filepath  An absolute (!) filepath to a file or directory (the target do
         *                  not need to exist). On Windows, both forward ("/") and
         *                  backward ("\") slashes are allowed as directory separators
         *                  (also mixed in one filepath). On other operating systems, only
         *                  forward slashes ("/") are allowed!. Also ".", ".." and
         *                  redundant directory separators are allowed.
         *
         * @return true on success, false on error (then this object will be invalid!)
         */
        bool setPath(const QString& filepath) noexcept;


        // Getters

        /**
         * @brief Check whether this object contains a valid filepath or not
         *
         * @return true if the filepath is valid, false if not
         */
        bool isValid() const noexcept {return mIsValid;}

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

        /**
         * @brief Check if the specified filepath is an existing, empty directory
         *
         * @return true if the path points to an existing, empty directory, false otherwise
         */
        bool isEmptyDir() const noexcept;

        /**
         * @brief Check if the specified filepath is the root directory
         *
         * @return True if the filepath is the filesystem root, false otherwise
         */
        bool isRoot() const noexcept;

        /**
         * @brief Check if the filepath is located inside another directory
         *
         * @return True if the filepath points to an item inside "dir", false otherwise
         */
        bool isLocatedInDir(const FilePath& dir) const noexcept;

        /**
         * @brief Get the absolute and well-formatted filepath as a QString
         *
         * @return  The absolute and well-formatted filepath, or an empty QString if this
         *          object is invalid.
         */
        QString toStr() const noexcept;

        /**
         * @brief Get the absolute filepath with native directory separators ("/" resp. "\")
         *
         * @return  The same as #toStr(), but with native separators (on other platforms
         *          than Windows, this method is identical to #toStr())
         */
        QString toNative() const noexcept;

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
         *          unchanged filepath)
         */
        FilePath toUnique() const noexcept;

        /**
         * @brief Convert an absolute filepath to a relative filepath (relative to another
         *        filepath)
         *
         * @param base  The base of the relative filepath (the part which will be removed
         *              from the absolute filepath). This must be a filepath to a
         *              directory, paths to a file will produce wrong results!
         *
         * @return  A relative filepath with "/" as directory separators (can contain "../")
         *
         * @note This method is very useful to store relative paths in (text) files.
         *
         * @see #fromRelative()
         */
        QString toRelative(const FilePath& base) const noexcept;

        /**
         * @brief Create and return a QUrl object with this filepath
         *
         * @return QUrl object which points to this local file (QUrl::fromLocalFile())
         */
        QUrl toQUrl() const noexcept {return QUrl::fromLocalFile(toStr());}

        /**
         * @brief Get the basename of the file or directory
         *
         * @return The filename before the first '.' character
         */
        QString getBasename() const noexcept;

        /**
         * @brief Get the complete basename of the file or directory
         *
         * @return The filename before the last '.' character
         */
        QString getCompleteBasename() const noexcept;

        /**
         * @brief Get the suffix of the file or directory
         *
         * @return The filename after the last '.' character
         */
        QString getSuffix() const noexcept;

        /**
         * @brief Get the complete suffix of the file or directory
         *
         * @return The filename after the first '.' character
         */
        QString getCompleteSuffix() const noexcept;

        /**
         * @brief Get the whole filename (without the path) of the file or directory
         *
         * @return The whole filename
         */
        QString getFilename() const noexcept;

        /**
         * @brief Get the filepath of the parent directory of the file or directory
         *
         * @return  A FilePath object with the parent directory (can be invalid if you try
         *          to get the parent directory of the filesystem root!)
         */
        FilePath getParentDir() const noexcept;

        /**
         * @brief Get the filepath to a file or directory which is relative to this filepath
         *
         * @param filename  A relative filepath to a file or directory,
         *                  like "file.txt" or "subdir/file.txt"
         *
         * @return A FilePath object to the specified file or directory
         *
         * @warning This method works only correct if this filepath represents a directory!
         *
         * @note This method is equal to FilePath#fromRelative(*this, filename);
         */
        FilePath getPathTo(const QString& filename) const noexcept;


        // General Methods

        /**
         * @brief Create all directories of this filepath (with all parent directories)
         *
         * @warning You should call this method only if this filepath represents a path to
         *          a directory! If it points to a file (which does not exist), this
         *          method will create a directory with the file's name (like "foo.txt")!
         *
         * @return true if success, false if not
         */
        bool mkPath() const noexcept;


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
         * @note This method is very useful to load relative paths from (text) files.
         *
         * @see #toRelative()
         */
        static FilePath fromRelative(const FilePath& base, const QString& relative) noexcept;

        /**
         * @brief Get the path to the temporary directory (e.g. "/tmp" on Unix/Linux)
         *
         * @return The filepath (in case of an error, the path can be invalid!)
         *
         * @todo test this method on windows and mac!
         */
        static FilePath getTempPath() noexcept;

        /**
         * @brief Get the path to the temporary application directory (e.g. "/tmp/librepcb")
         *
         * @return The filepath (in case of an error, the path can be invalid!)
         *
         * @todo test this method on windows and mac!
         */
        static FilePath getApplicationTempPath() noexcept;

        /**
         * @brief Get a random temporary directory path (e.g. "/tmp/librepcb/42")
         *
         * @return The random filepath (in case of an error, the path can be invalid!)
         */
        static FilePath getRandomTempPath() noexcept;

        /**
         * @brief Clean a given string so that it becomes a valid filename
         *
         * Every time a file- or directory name needs to be constructed (e.g. from user
         * input), you must use this function to replace/remove all characters which are
         * not allowed for file/dir paths.
         *
         * These are the only allowed characters: A–Z a–z 0–9 . _ -
         *
         * In addition, the length of the filename will be limited to 120 characters.
         *
         * @param userInput An arbitrary string (may be directly from a user input field)
         * @param options   Some options to define how the filename should be escaped
         *
         * @return A string which is either empty or a valid filename (based on userInput)
         *
         * @note    This function does exectly the same on all supported platforms, even
         *          if the set of allowed characters depends on the platform. This way we
         *          can guarantee that all created files/directories are platform
         *          independent.
         */
        static QString cleanFileName(const QString& userInput, CleanFileNameOptions options) noexcept;


        // Operator Overloadings

        /**
         * @brief The assign operator to copy a FilePath into another FilePath object
         */
        FilePath& operator=(const FilePath& rhs) noexcept;

        /**
         * @brief The "==" operator to compare two FilePath objects
         *
         * @note This method compares the return values of #toStr() of both objects.
         *
         * @return true if both filepaths are identical, false otherwise
         */
        bool operator==(const FilePath& rhs) const noexcept;

        /**
         * @brief The "!=" operator to compare two FilePath objects
         *
         * @note This method compares the return values of #toStr() of both objects.
         *
         * @return false if both filepaths are identical, true otherwise
         */
        bool operator!=(const FilePath& rhs) const noexcept;


    private:

        // Private Methods

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

        bool mIsValid;
        QFileInfo mFileInfo; ///< the absolute and well-formatted filepath in a QFileInfo
};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const FilePath& filepath);
QDebug& operator<<(QDebug& stream, const FilePath& filepath);

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

// QFlags
Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::FilePath::CleanFileNameOptions)

#endif // LIBREPCB_FILEPATH_H
