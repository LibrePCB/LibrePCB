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

#ifndef WORKSPACESETTINGSDIALOG_H
#define WORKSPACESETTINGSDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class WorkspaceSettings;

namespace Ui {
class WorkspaceSettingsDialog;
}

/*****************************************************************************************
 *  Class WorkspaceSettingsDialog
 ****************************************************************************************/

/**
 * @brief The WorkspaceSettingsDialog class
 *
 * This dialog class implements a GUI for all workspace settings. An instance of
 * WorkspaceSettingsDialog is created in the class WorkspaceSettings. There must not exist
 * more than one instance of this class at the same time in the same application instance!
 *
 * @author ubruhin
 * @date 2014-07-12
 */
class WorkspaceSettingsDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WorkspaceSettingsDialog(WorkspaceSettings& settings);
        ~WorkspaceSettingsDialog();

    protected:

        // Inherited from QDialog
        void accept();
        void reject();

    private slots:

        // Private Slots for the GUI elements
        void on_buttonBox_clicked(QAbstractButton *button);

    private:

        // make some methods inaccessible...
        WorkspaceSettingsDialog();
        WorkspaceSettingsDialog(const WorkspaceSettingsDialog& other);
        WorkspaceSettingsDialog& operator=(const WorkspaceSettingsDialog& rhs);


        // General Attributes
        Ui::WorkspaceSettingsDialog* mUi;
        WorkspaceSettings& mSettings; ///< a pointer to the WorkspaceSettings object
};

#endif // WORKSPACESETTINGSDIALOG_H
