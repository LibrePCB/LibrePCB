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

#ifndef LIBREPCB_PROJECT_CMDPROJECTSETTINGSCHANGE_H
#define LIBREPCB_PROJECT_CMDPROJECTSETTINGSCHANGE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class ProjectSettings;

/*****************************************************************************************
 *  Class CmdProjectSettingsChange
 ****************************************************************************************/

/**
 * @brief The CmdProjectSettingsChange class
 */
class CmdProjectSettingsChange final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdProjectSettingsChange(ProjectSettings& settings) noexcept;
        ~CmdProjectSettingsChange() noexcept;

        // Setters
        void restoreDefaults() noexcept;
        void setLocaleOrder(const QStringList& locales) noexcept;
        void setNormOrder(const QStringList& norms) noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        void performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;

        void applyNewSettings() throw (Exception);
        void applyOldSettings() throw (Exception);


        // Attributes from the constructor
        ProjectSettings& mSettings;

        // Old/New Settings
        bool mRestoreDefaults;
        QStringList mLocaleOrderOld;
        QStringList mLocaleOrderNew;
        QStringList mNormOrderOld;
        QStringList mNormOrderNew;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDPROJECTSETTINGSCHANGE_H
