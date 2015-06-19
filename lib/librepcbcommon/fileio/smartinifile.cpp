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
#include "smartinifile.h"
#include "../exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartIniFile::SmartIniFile(const FilePath& filepath, bool restore, bool readOnly,
                           bool create, int expectedVersion, int createVersion) throw (Exception) :
    SmartFile(filepath, restore, readOnly, create),
    mTmpIniFilePath(), mSettings(), mFileVersion(-1)
{
    // create a unique filename in the operating system's temporary directory
    QString tmpFilename = QCryptographicHash::hash(mOpenedFilePath.toStr().toLocal8Bit(),
                            QCryptographicHash::Sha256).toBase64(
                            QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    mTmpIniFilePath.setPath(QDir::temp().absoluteFilePath("LibrePCB/" % tmpFilename));
    mTmpIniFilePath.getParentDir().mkPath();

    // Remove the temporary file if it exists already
    if (mTmpIniFilePath.isExistingFile())
    {
        if (!QFile::remove(mTmpIniFilePath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__, mTmpIniFilePath.toStr(),
                QString(tr("Could not remove file \"%1\"!")).arg(mTmpIniFilePath.toNative()));
        }
    }

    if (mIsCreated)
    {
        // create a new temporary INI file
        QSettings* s = createQSettings();
        s->clear();
        releaseQSettings(s);
        setFileVersion(createVersion);
    }
    else
    {
        // Copy the INI file to the temporary file
        if (!QFile::copy(mOpenedFilePath.toStr(), mTmpIniFilePath.toStr()))
        {
            throw RuntimeError(__FILE__, __LINE__,
                QString("%1:%2").arg(mOpenedFilePath.toStr(), mTmpIniFilePath.toStr()),
                QString(tr("Could not copy file \"%1\" to \"%2\"!"))
                .arg(mOpenedFilePath.toNative(), mTmpIniFilePath.toNative()));
        }
    }

    // Read the file version
    QSettings* s = createQSettings();
    bool ok;
    mFileVersion = s->value("meta/file_version").toInt(&ok);
    if (!ok) mFileVersion = -1;
    releaseQSettings(s);

    // check the file version number
    if ((expectedVersion > -1) && (mFileVersion != expectedVersion))
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Invalid file version in \"%1\": %2 (expected: %3)"))
            .arg(mOpenedFilePath.toNative()).arg(mFileVersion).arg(expectedVersion));
    }
}

SmartIniFile::~SmartIniFile() noexcept
{
    if (!mSettings.isEmpty())
    {
        qWarning() << "mSettings still contains" << mSettings.count() << "elements!";
        qDeleteAll(mSettings);      mSettings.clear();
    }

    // remove temporary file in the tmp directory
    QFile::remove(mTmpIniFilePath.toStr());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SmartIniFile::setFileVersion(int version) throw (Exception)
{
    QSettings* s = createQSettings();
    // Maybe we do not need to convert the integer to a QString explicitely because
    // QVariant can do this also. But as the method QString::number(int) is
    // locale-independent for sure, we use that method to be on the save site.
    s->setValue("meta/file_version", QString::number(version));
    mFileVersion = version;
    releaseQSettings(s);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QSettings* SmartIniFile::createQSettings() throw (Exception)
{
    QSettings* settings = 0;

    try
    {
        settings = new QSettings(mTmpIniFilePath.toStr(), QSettings::IniFormat);
        if (settings->status() != QSettings::NoError)
        {
            throw RuntimeError(__FILE__, __LINE__, mTmpIniFilePath.toStr(),
                QString(tr("Error while opening file \"%1\"")).arg(mTmpIniFilePath.toNative()));
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

void SmartIniFile::releaseQSettings(QSettings* settings) noexcept
{
    Q_ASSERT(settings);
    Q_ASSERT(mSettings.contains(settings));

    settings->sync();
    if (settings->status() == QSettings::NoError)
    {
        mSettings.removeOne(settings);
        delete settings;
    }

    // if sync() was not successful, we let the QSettings object alive and hold the
    // pointer in the mSettings list. This way, the method save() can also detect the error
    // and then throws an exception. The destructor then will delete the QSettings object.
}

void SmartIniFile::save(bool toOriginal) throw (Exception)
{
    const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);

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
    if (!QFile::copy(mTmpIniFilePath.toStr(), filepath.toStr()))
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1:%2").arg(mTmpIniFilePath.toStr(), filepath.toStr()),
            QString(tr("Could not copy file \"%1\" to \"%2\"!"))
            .arg(mTmpIniFilePath.toNative(), filepath.toNative()));
    }

    // check if the target file exists
    if (!filepath.isExistingFile())
    {
        throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
            QString(tr("Error while writing to file \"%1\"!")).arg(filepath.toNative()));
    }

    updateMembersAfterSaving(toOriginal);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartIniFile* SmartIniFile::create(const FilePath& filepath, int version) throw (Exception)
{
    return new SmartIniFile(filepath, false, false, true, -1, version);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
