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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FilePath::FilePath(const QString& filepath) throw (Exception)
{
    FilePath::setPathEx(filepath);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool FilePath::setPath(const QString& filepath) noexcept
{
    try {setPathEx(filepath);}
    catch (Exception&) {return false;}
    return true;
}

void FilePath::setPathEx(const QString& filepath) throw (Exception)
{
    QString newPath = makeWellFormatted(filepath);

    // "newPath" is our new filepath, let's check if it is absolute
    if (!QDir::isAbsolutePath(newPath))
    {
        throw RuntimeError(QString("\"%1\" is not an absolute filepath!").arg(newPath),
                           __FILE__, __LINE__);
    }

    // all ok --> apply the new filepath
    mPath = newPath;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString FilePath::toUnique() const noexcept
{
    QFileInfo info(mPath);
    QString unique = makeWellFormatted(info.canonicalFilePath());
    if (!unique.isEmpty())
        return unique;
    else
        return mPath;
}

QString FilePath::toRelative(const FilePath& base) const noexcept
{
    QDir baseDir(base.mPath);
    return makeWellFormatted(baseDir.relativeFilePath(mPath));
}

bool FilePath::isExistingFile() const noexcept
{
    QFileInfo info(mPath);
    return (info.isFile() && info.exists());
}

bool FilePath::isExistingDir() const noexcept
{
    QFileInfo info(mPath);
    return (info.isDir() && info.exists());
}

/*****************************************************************************************
 *  Operators
 ****************************************************************************************/

FilePath& FilePath::operator=(const FilePath& rhs) noexcept
{
    mPath = rhs.mPath;
    return *this;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

FilePath FilePath::fromRelative(const FilePath& base, const QString& relative)
{
    QDir baseDir(base.mPath);
    return FilePath(baseDir.absoluteFilePath(relative));
}

QString FilePath::makeWellFormatted(const QString& filepath) noexcept
{
    // change all separators to "/", remove redundant separators, resolve "." and "..".
    QString newPath = QDir::cleanPath(filepath);

    // tests show that QDir::cleanPath() will also remove a slash at the end of the
    // filepath, but as this feature is not described in the documentation, we will do
    // this again to ensure that this will always work correctly!
    while ((newPath.endsWith("/")) && (newPath != "/")) // the last character is "/"
        newPath.chop(1); // remove the last character

    return newPath;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
