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
#include "cmdprojectsetmetadata.h"
#include "../project.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdProjectSetMetadata::CmdProjectSetMetadata(Project& project, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Change Project Metadata"), parent),
    mProject(project), mRedoOrUndoCalled(false),
    mOldName(mProject.getName()),               mNewName(mProject.getName()),
    mOldDescription(mProject.getDescription()), mNewDescription(mProject.getDescription()),
    mOldAuthor(mProject.getAuthor()),           mNewAuthor(mProject.getAuthor()),
    mOldCreated(mProject.getCreated()),         mNewCreated(mProject.getCreated())
{
}

CmdProjectSetMetadata::~CmdProjectSetMetadata() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdProjectSetMetadata::setName(const QString& newName) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mNewName = newName;
}

void CmdProjectSetMetadata::setDescription(const QString& newDescription) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mNewDescription = newDescription;
}

void CmdProjectSetMetadata::setAuthor(const QString& newAuthor) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mNewAuthor = newAuthor;
}

void CmdProjectSetMetadata::setCreated(const QDateTime& newCreated) noexcept
{
    Q_ASSERT(mRedoOrUndoCalled == false);
    mNewCreated = newCreated;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdProjectSetMetadata::redo() throw (Exception)
{
    mRedoOrUndoCalled = true;

    if (mNewName != mOldName)
        mProject.setName(mNewName);
    if (mNewDescription != mOldDescription)
        mProject.setDescription(mNewDescription);
    if (mNewAuthor != mOldAuthor)
        mProject.setAuthor(mNewAuthor);
    if (mNewCreated != mOldCreated)
        mProject.setCreated(mNewCreated);

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        if (mNewName != mOldName)
            mProject.setName(mOldName);
        if (mNewDescription != mOldDescription)
            mProject.setDescription(mOldDescription);
        if (mNewAuthor != mOldAuthor)
            mProject.setAuthor(mOldAuthor);
        if (mNewCreated != mOldCreated)
            mProject.setCreated(mOldCreated);
        throw;
    }
}

void CmdProjectSetMetadata::undo() throw (Exception)
{
    mRedoOrUndoCalled = true;

    if (mNewName != mOldName)
        mProject.setName(mOldName);
    if (mNewDescription != mOldDescription)
        mProject.setDescription(mOldDescription);
    if (mNewAuthor != mOldAuthor)
        mProject.setAuthor(mOldAuthor);
    if (mNewCreated != mOldCreated)
        mProject.setCreated(mOldCreated);

    try
    {
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        if (mNewName != mOldName)
            mProject.setName(mNewName);
        if (mNewDescription != mOldDescription)
            mProject.setDescription(mNewDescription);
        if (mNewAuthor != mOldAuthor)
            mProject.setAuthor(mNewAuthor);
        if (mNewCreated != mOldCreated)
            mProject.setCreated(mNewCreated);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
