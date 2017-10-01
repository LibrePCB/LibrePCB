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
#include "cmdprojectmetadataedit.h"
#include "../projectmetadata.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdProjectMetadataEdit::CmdProjectMetadataEdit(ProjectMetadata& metadata) noexcept :
    UndoCommand(tr("Edit Project Metadata")),
    mMetadata(metadata),
    mOldName(mMetadata.getName()),               mNewName(mMetadata.getName()),
    mOldAuthor(mMetadata.getAuthor()),           mNewAuthor(mMetadata.getAuthor()),
    mOldVersion(mMetadata.getVersion()),         mNewVersion(mMetadata.getVersion()),
    mOldAttributes(mMetadata.getAttributes()),   mNewAttributes(mOldAttributes)
{
}

CmdProjectMetadataEdit::~CmdProjectMetadataEdit() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdProjectMetadataEdit::setName(const QString& newName) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewName = newName;
}

void CmdProjectMetadataEdit::setAuthor(const QString& newAuthor) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewAuthor = newAuthor;
}

void CmdProjectMetadataEdit::setVersion(const QString& newVersion) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewVersion = newVersion;
}

void CmdProjectMetadataEdit::setAttributes(const AttributeList& attributes) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewAttributes = attributes;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdProjectMetadataEdit::performExecute()
{
    performRedo(); // can throw

    if (mNewName != mOldName)               return true;
    if (mNewAuthor != mOldAuthor)           return true;
    if (mNewVersion != mOldVersion)         return true;
    if (mNewAttributes != mOldAttributes)   return true;
    return false;
}

void CmdProjectMetadataEdit::performUndo()
{
    mMetadata.setName(mOldName);
    mMetadata.setAuthor(mOldAuthor);
    mMetadata.setVersion(mOldVersion);
    mMetadata.setAttributes(mOldAttributes);
}

void CmdProjectMetadataEdit::performRedo()
{
    mMetadata.setName(mNewName);
    mMetadata.setAuthor(mNewAuthor);
    mMetadata.setVersion(mNewVersion);
    mMetadata.setAttributes(mNewAttributes);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
