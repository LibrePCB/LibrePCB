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

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartVersionFile::SmartVersionFile(const FilePath& filepath, bool restore, bool readOnly,
                                   bool create, const Version& newVersion) throw (Exception) :
    SmartFile(filepath, restore, readOnly, create), mVersion()
{
    if (mIsCreated) {
        mVersion = newVersion;
    }
    else {
        // read the content of the file
        QString content = QString(readContentFromFile(mOpenedFilePath));
        QStringList lines = content.split("\n", QString::KeepEmptyParts);
        mVersion.setVersion((lines.count() > 0) ? lines.first() : QString());
        if (!mVersion.isValid()) {
            throw RuntimeError(__FILE__, __LINE__, content,
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

void SmartVersionFile::save(bool toOriginal) throw (Exception)
{
    if (mVersion.isValid()) {
        const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);
        saveContentToFile(filepath, QString("%1\n").arg(mVersion.toStr()).toUtf8());
        updateMembersAfterSaving(toOriginal);
    } else {
        throw LogicError(__FILE__, __LINE__, mVersion.toStr(), tr("Invalid version number"));
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartVersionFile* SmartVersionFile::create(const FilePath& filepath, const Version& version) throw (Exception)
{
    return new SmartVersionFile(filepath, false, false, true, version);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
