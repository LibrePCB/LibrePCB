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
#include "inifile.h"
#include "exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

IniFile::IniFile(const FilePath& filepath, bool restore) throw (Exception) :
    QObject(0), mFilepath(filepath)
{
    // decide if we open the original file (*.ini) or the backup (*.ini~)
    FilePath iniFilepath(mFilepath.toStr() % '~');
    if ((!restore) || (!iniFilepath.isExistingFile()))
        iniFilepath = mFilepath;

    // check if the file exists
    if (!iniFilepath.isExistingFile())
    {
        throw RuntimeError(__FILE__, __LINE__, iniFilepath.toStr(),
            QString(tr("The file \"%1\" does not exist!")).arg(iniFilepath.toNative()));
    }

    // create a unique filename in the operating system's temporary directory
    QString tmpFilename = QCryptographicHash::hash(iniFilepath.toStr().toLocal8Bit(),
                            QCryptographicHash::Sha256).toBase64(
                            QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    mTmpFilepath.setPath(QDir::temp().absoluteFilePath("EDA4U/" % tmpFilename));
    mTmpFilepath.getParentDir().mkPath();

    // Remove the temporary file if it exists already
    if (mTmpFilepath.isExistingFile())
    {
        if (!QFile::remove(mTmpFilepath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__, mTmpFilepath.toStr(),
                QString(tr("Could not remove file \"%1\"!")).arg(mTmpFilepath.toNative()));
        }
    }

    // Copy the INI file to the temporary file
    if (!QFile::copy(iniFilepath.toStr(), mTmpFilepath.toStr()))
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1:%2").arg(iniFilepath.toStr(), mTmpFilepath.toStr()),
            QString(tr("Could not copy file \"%1\" to \"%2\"!"))
            .arg(iniFilepath.toNative(), mTmpFilepath.toNative()));
    }
}

IniFile::~IniFile() noexcept
{
    qDeleteAll(mSettings);      mSettings.clear();

    // remove temporary files
    QFile::remove(mTmpFilepath.toStr());
    QFile::remove(mFilepath.toStr() % "~");
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QSettings* IniFile::createQSettings() throw (Exception)
{
    QSettings* settings = 0;

    try
    {
        settings = new QSettings(mTmpFilepath.toStr(), QSettings::IniFormat);
        if (settings->status() != QSettings::NoError)
        {
            throw RuntimeError(__FILE__, __LINE__, mTmpFilepath.toStr(),
                QString(tr("Error while opening file \"%1\"!")).arg(mTmpFilepath.toNative()));
        }
    }
    catch (Exception& e)
    {
        delete settings;    settings = 0;
        throw;
    }

    mSettings.append(settings);
    return settings;
}

void IniFile::releaseQSettings(QSettings* settings) noexcept
{
    Q_CHECK_PTR(settings);
    Q_ASSERT(mSettings.contains(settings));

    mSettings.removeOne(settings);
    delete settings;
}

void IniFile::remove() const throw (Exception)
{
    bool success = true;

    if (QFile::exists(mFilepath.toStr()))
    {
        if (!QFile::remove(mFilepath.toStr()))
            success = false;
    }

    if (QFile::exists(mFilepath.toStr() % '~'))
    {
        if (!QFile::remove(mFilepath.toStr() % '~'))
            success = false;
    }

    if (mSettings.isEmpty())
    {
        if (QFile::exists(mTmpFilepath.toStr()))
        {
            if (!QFile::remove(mTmpFilepath.toStr()))
                success = false;
        }
    }
    else
        qWarning() << "mSettings is not empty!";

    if (!success)
    {
        throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(),
            QString(tr("Could not remove file \"%1\"")).arg(mFilepath.toNative()));
    }
}

void IniFile::save(bool toOriginal) throw (Exception)
{
    FilePath filepath(toOriginal ? mFilepath.toStr() : mFilepath.toStr() % '~');

    foreach (QSettings* settings, mSettings)
    {
        settings->sync(); // write all changes to the temp file in temp directory

        if (settings->status() != QSettings::NoError)
        {
            throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
                QString(tr("Error while writing to file \"%1\"!")).arg(filepath.toNative()));
        }
    }

    // remove the target file
    if (filepath.isExistingFile())
    {
        if (!QFile::remove(filepath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
                QString(tr("Could not remove file \"%1\"!")).arg(filepath.toNative()));
        }
    }

    // copy the temp file in the temp directory to the original directory
    if (!QFile::copy(mTmpFilepath.toStr(), filepath.toStr()))
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1:%2").arg(mTmpFilepath.toStr(), filepath.toStr()),
            QString(tr("Could not copy file \"%1\" to \"%2\"!"))
            .arg(mTmpFilepath.toNative(), filepath.toNative()));
    }

    // check if the target file exists
    if (!filepath.isExistingFile())
    {
        throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
            QString(tr("Error while writing to file \"%1\"!")).arg(filepath.toNative()));
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

IniFile* IniFile::create(const FilePath& filepath) throw (Exception)
{
    // remove the file if it exists already
    if (filepath.isExistingFile())
    {
        if (!QFile::remove(filepath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
                QString(tr("Cannot remove file \"%1\"")).arg(filepath.toNative()));
        }
    }

    // create an empty file
    QFile file(filepath.toStr());
    if (!file.open(QIODevice::WriteOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, filepath.toStr(), QString(tr("Cannot "
            "create file \"%1\": %2")).arg(filepath.toNative(), file.errorString()));
    }
    file.close();

    // open and return the new INI file object
    return new IniFile(filepath, false);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
