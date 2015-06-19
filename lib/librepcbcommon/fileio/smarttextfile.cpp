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
#include "smarttextfile.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SmartTextFile::SmartTextFile(const FilePath& filepath, bool restore, bool readOnly, bool create) throw (Exception) :
    SmartFile(filepath, restore, readOnly, create)
{
    if (mIsCreated)
    {
        // nothing to do, leave "mContent" empty
    }
    else
    {
        // read the content of the file
        mContent = readContentFromFile(mOpenedFilePath);
    }
}

SmartTextFile::~SmartTextFile()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SmartTextFile::save(bool toOriginal) throw (Exception)
{
    const FilePath& filepath = prepareSaveAndReturnFilePath(toOriginal);
    saveContentToFile(filepath, mContent);
    updateMembersAfterSaving(toOriginal);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

SmartTextFile* SmartTextFile::create(const FilePath& filepath) throw (Exception)
{
    return new SmartTextFile(filepath, false, false, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
