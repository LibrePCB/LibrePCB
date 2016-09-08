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
#include "cmdprojectsetmetadata.h"
#include "../project.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdProjectSetMetadata::CmdProjectSetMetadata(Project& project) noexcept :
    UndoCommand(tr("Change Project Metadata")),
    mProject(project),
    mOldName(mProject.getName()),               mNewName(mProject.getName()),
    mOldAuthor(mProject.getAuthor()),           mNewAuthor(mProject.getAuthor()),
    mOldVersion(mProject.getVersion()),         mNewVersion(mProject.getVersion())
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
    Q_ASSERT(!wasEverExecuted());
    mNewName = newName;
}

void CmdProjectSetMetadata::setAuthor(const QString& newAuthor) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewAuthor = newAuthor;
}

void CmdProjectSetMetadata::setVersion(const QString& newVersion) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewVersion = newVersion;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdProjectSetMetadata::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true; // TODO: determine if the metadata was really modified
}

void CmdProjectSetMetadata::performUndo() throw (Exception)
{
    mProject.setName(mOldName);
    mProject.setAuthor(mOldAuthor);
    mProject.setVersion(mOldVersion);
}

void CmdProjectSetMetadata::performRedo() throw (Exception)
{
    mProject.setName(mNewName);
    mProject.setAuthor(mNewAuthor);
    mProject.setVersion(mNewVersion);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
