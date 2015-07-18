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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "smartfile.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartFile::SmartFile(const FilePath& filepath, bool restore, bool readOnly, bool create) throw (Exception) :
    mFilePath(filepath), mTmpFilePath(filepath.toStr() % '~'),
    mOpenedFilePath(filepath), mIsRestored(restore), mIsReadOnly(readOnly),
    mIsCreated(create)
{
    if (create)
    {
        Q_ASSERT(mIsRestored == false);
        Q_ASSERT(mIsReadOnly == false);

        // remove the files if they exists already
        if (mFilePath.isExistingFile())
        {
            if (!QFile::remove(mFilePath.toStr()))
            {
                throw RuntimeError(__FILE__, __LINE__, mFilePath.toStr(),
                    QString(tr("Cannot remove file \"%1\"")).arg(mFilePath.toNative()));
            }
        }
        if (mTmpFilePath.isExistingFile())
        {
            if (!QFile::remove(mTmpFilePath.toStr()))
            {
                throw RuntimeError(__FILE__, __LINE__, mTmpFilePath.toStr(),
                    QString(tr("Cannot remove file \"%1\"")).arg(mTmpFilePath.toNative()));
            }
        }
    }
    else
    {
        // decide if we open the original file (*.*) or the backup (*.*~)
        if ((mIsRestored) && (mTmpFilePath.isExistingFile()))
            mOpenedFilePath = mTmpFilePath;

        // check if the file exists
        if (!mOpenedFilePath.isExistingFile())
        {
            throw RuntimeError(__FILE__, __LINE__, mOpenedFilePath.toStr(),
                QString(tr("The file \"%1\" does not exist!")).arg(mOpenedFilePath.toNative()));
        }
    }
}

SmartFile::~SmartFile() noexcept
{
    if ((!mIsRestored) && (!mIsReadOnly) && (mTmpFilePath.isExistingFile()))
    {
        // remove temporary file
        QFile::remove(mTmpFilePath.toStr());
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SmartFile::removeFile(bool original) throw (Exception)
{
    if (mIsReadOnly)
        throw LogicError(__FILE__, __LINE__, QString(), tr("Cannot remove read-only file!"));

    FilePath filepath(original ? mFilePath : mTmpFilePath);

    if (filepath.isExistingFile())
    {
        if (!QFile::remove(filepath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
                QString(tr("Cannot remove file \"%1\"")).arg(filepath.toNative()));
        }
    }
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

const FilePath& SmartFile::prepareSaveAndReturnFilePath(bool toOriginal) throw (Exception)
{
    if (mIsReadOnly)
        throw LogicError(__FILE__, __LINE__, QString(), tr("Cannot save read-only file!"));

    const FilePath& filepath(toOriginal ? mFilePath : mTmpFilePath);

    if (!filepath.getParentDir().isExistingDir())
    {
        // try to create parent directories
        if (!filepath.getParentDir().mkPath())
            qWarning() << "could not make path for file" << filepath.toNative();
    }

    return filepath;
}

void SmartFile::updateMembersAfterSaving(bool toOriginal) noexcept
{
    if (toOriginal && mIsRestored)
        mIsRestored = false;

    if (toOriginal && mIsCreated)
        mIsCreated = false;
}

QByteArray SmartFile::readContentFromFile(const FilePath& filepath) throw (Exception)
{
    QFile file(filepath.toStr());
    if (!file.open(QIODevice::ReadOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, filepath.toStr(), QString(tr("Cannot "
            "open file \"%1\": %2")).arg(filepath.toNative(), file.errorString()));
    }
    return file.readAll();
}

void SmartFile::saveContentToFile(const FilePath& filepath, const QByteArray& content) throw (Exception)
{
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
