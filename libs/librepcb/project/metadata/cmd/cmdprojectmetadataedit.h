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

#ifndef LIBREPCB_PROJECT_CMDPROJECTMETADATAEDIT_H
#define LIBREPCB_PROJECT_CMDPROJECTMETADATAEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommand.h>
#include "../projectmetadata.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Class CmdProjectMetadataEdit
 ****************************************************************************************/

/**
 * @brief The CmdProjectMetadataEdit class
 */
class CmdProjectMetadataEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdProjectMetadataEdit(ProjectMetadata& metadata) noexcept;
        ~CmdProjectMetadataEdit() noexcept;

        // Setters
        void setName(const ElementName& newName) noexcept;
        void setAuthor(const QString& newAuthor) noexcept;
        void setVersion(const QString& newVersion) noexcept;
        void setAttributes(const AttributeList& attributes) noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        // General
        ProjectMetadata& mMetadata;

        // Misc
        ElementName mOldName;
        ElementName mNewName;
        QString mOldAuthor;
        QString mNewAuthor;
        QString mOldVersion;
        QString mNewVersion;
        AttributeList mOldAttributes;
        AttributeList mNewAttributes;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDPROJECTMETADATAEDIT_H
