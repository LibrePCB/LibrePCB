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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "smartversionfile.h"
#include "fileutils.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartVersionFile::SmartVersionFile(const FilePath& filepath, bool restore, bool readOnly,
                                   bool create, const Version& newVersion) :
    SmartFile(filepath, restore, readOnly, create), mVersion()
{
    if (mIsCreated) {
        mVersion = newVersion;
    }
    else {
        // read the content of the file
        QString content = QString(FileUtils::readFile(mOpenedFilePath));
        QStringList lines = content.split("\n", QString::KeepEmptyParts);
        mVersion.setVersion((lines.count() > 0) ? lines.first() : QString());
        if (!mVersion.isValid()) {
            qDebug() << "content:" << content;
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Invalid version number in file \"%1\".")).arg(filepath.toNative()));
        }
    }
}

SmartVersionFile::~SmartVersionFile() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SmartVersionFile::save(bool toOriginal)
{
    if (mVersion.isValid()) {
        const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);
        FileUtils::writeFile(filepath, QString("%1\n").arg(mVersion.toStr()).toUtf8());
        updateMembersAfterSaving(toOriginal);
    } else {
        qDebug() << mVersion.toStr();
        throw LogicError(__FILE__, __LINE__, tr("Invalid version number"));
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartVersionFile* SmartVersionFile::create(const FilePath& filepath, const Version& version)
{
    return new SmartVersionFile(filepath, false, false, true, version);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
