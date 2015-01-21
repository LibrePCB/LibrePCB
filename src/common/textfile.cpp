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
#include "textfile.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

TextFile::TextFile(const FilePath& filepath, bool restore, bool readOnly) throw (Exception) :
    QObject(0), mFilePath(filepath), mIsRestored(restore), mIsReadOnly(readOnly)
{
    // decide if we open the original file (*.*) or the backup (*.*~)
    mOpenedFilePath.setPath(mFilePath.toStr() % '~');
    if ((!mIsRestored) || (!mOpenedFilePath.isExistingFile()))
        mOpenedFilePath = mFilePath;

    // check if the file exists
    if (!mOpenedFilePath.isExistingFile())
    {
        throw RuntimeError(__FILE__, __LINE__, mOpenedFilePath.toStr(),
            QString(tr("The file \"%1\" does not exist!")).arg(mOpenedFilePath.toNative()));
    }

    // try opening the file
    QFile file(mOpenedFilePath.toStr());
    if (!file.open(QIODevice::ReadOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, mOpenedFilePath.toStr(), QString(tr("Cannot "
            "open file \"%1\": %2")).arg(mOpenedFilePath.toNative(), file.errorString()));
    }

    mContent = file.readAll();
}

TextFile::~TextFile()
{
    if ((!mIsRestored) && (!mIsReadOnly))
    {
        // remove temporary file
        QFile::remove(mFilePath.toStr() % "~");
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void TextFile::remove() const throw (Exception)
{
    bool success = true;

    if (mIsReadOnly)
        throw LogicError(__FILE__, __LINE__, QString(), tr("Cannot remove read-only file!"));

    if (QFile::exists(mFilePath.toStr()))
    {
        if (!QFile::remove(mFilePath.toStr()))
            success = false;
    }

    if (QFile::exists(mFilePath.toStr() % '~'))
    {
        if (!QFile::remove(mFilePath.toStr() % '~'))
            success = false;
    }

    if (!success)
    {
        throw RuntimeError(__FILE__, __LINE__, mFilePath.toStr(),
            QString(tr("Could not remove file \"%1\"")).arg(mFilePath.toNative()));
    }
}

void TextFile::save(bool toOriginal) throw (Exception)
{
    if (mIsReadOnly)
        throw LogicError(__FILE__, __LINE__, QString(), tr("Cannot save read-only file!"));

    FilePath filepath(toOriginal ? mFilePath.toStr() : mFilePath.toStr() % '~');
    saveContentToFile(filepath, mContent);

    if (toOriginal && mIsRestored)
        mIsRestored = false;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

TextFile* TextFile::create(const FilePath& filepath) throw (Exception)
{
    FilePath tmpFilepath(filepath.toStr() % '~');

    // remove the file if it exists already
    if (filepath.isExistingFile())
    {
        if (!QFile::remove(filepath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
                QString(tr("Cannot remove file \"%1\"")).arg(filepath.toNative()));
        }
    }

    // create empty temporary file
    saveContentToFile(tmpFilepath, QByteArray());

    return new TextFile(filepath, true, false);
}

void TextFile::saveContentToFile(const FilePath& filepath, const QByteArray& content) throw (Exception)
{
    if (!filepath.getParentDir().isExistingDir())
    {
        // try to create parent directories
        if (!filepath.getParentDir().mkPath())
            qWarning() << "could not make path for file" << filepath.toNative();
    }

    QSaveFile file(filepath.toStr());
    if (!file.open(QIODevice::WriteOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3]")
            .arg(filepath.toStr(), file.errorString()).arg(file.error()),
            QString(tr("Could not open or create file \"%1\": %2"))
            .arg(filepath.toNative(), file.errorString()));
    }

    qint64 written = file.write(content);
    if (written != content.size())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1: %2 (only %3 of %4 bytes written)")
            .arg(filepath.toStr(), file.errorString()).arg(written).arg(content.size()),
            QString(tr("Could not write to file \"%1\": %2"))
            .arg(filepath.toNative(), file.errorString()));
    }

    if (!file.commit())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr("Could not write to "
            "file \"%1\": %2")).arg(filepath.toNative(), file.errorString()));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
