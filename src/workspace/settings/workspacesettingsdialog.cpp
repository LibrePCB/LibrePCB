/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QtWidgets>
#include "workspacesettingsdialog.h"
#include "ui_workspacesettingsdialog.h"
#include "../workspace.h"
#include "workspacesettings.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettingsDialog::WorkspaceSettingsDialog(Workspace& workspace,
                                                 WorkspaceSettings& settings) :
    QDialog(0), mUi(new Ui::WorkspaceSettingsDialog), mWorkspace(workspace),
    mSettings(settings)
{
    mUi->setupUi(this);

    // Add all settings widgets

    // tab: general
    mUi->generalLayout->addRow(mSettings.getAppLocale()->getLabelText(),
                               mSettings.getAppLocale()->getWidget());
    mUi->generalLayout->addRow(mSettings.getAppDefMeasUnit()->getLabelText(),
                               mSettings.getAppDefMeasUnit()->getComboBox());
    mUi->generalLayout->addRow(mSettings.getProjectAutosaveInterval()->getLabelText(),
                               mSettings.getProjectAutosaveInterval()->getWidget());

    // tab: library
    mUi->libraryLayout->addRow(mSettings.getLibLocaleOrder()->getLabelText(),
                               mSettings.getLibLocaleOrder()->getWidget());

    // load the window geometry
    QSettings clientSettings;
    restoreGeometry(clientSettings.value("workspace_settings_dialog/window_geometry").toByteArray());
}

WorkspaceSettingsDialog::~WorkspaceSettingsDialog()
{
    // save the window geometry
    QSettings clientSettings;
    clientSettings.setValue("workspace_settings_dialog/window_geometry", saveGeometry());

    // Remove all settings widgets from the dialog (important for memory management!)

    // tab: general
    mSettings.getAppLocale()->getWidget()->setParent(0);
    mSettings.getAppDefMeasUnit()->getComboBox()->setParent(0);
    mSettings.getProjectAutosaveInterval()->getWidget()->setParent(0);

    // tab: library
    mSettings.getLibLocaleOrder()->getWidget()->setParent(0);

    // delete private member objects
    delete mUi;         mUi = 0;
}

/*****************************************************************************************
 *  Inherited from QDialog
 ****************************************************************************************/

void WorkspaceSettingsDialog::accept()
{
    mSettings.applyAll();
    QDialog::accept();
}

void WorkspaceSettingsDialog::reject()
{
    mSettings.revertAll();
    QDialog::reject();
}

/*****************************************************************************************
 *  Private Slots for the GUI elements
 ****************************************************************************************/

void WorkspaceSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (mUi->buttonBox->buttonRole(button))
    {
        case QDialogButtonBox::AcceptRole:
        case QDialogButtonBox::ApplyRole:
            mSettings.applyAll();
            break;

        case QDialogButtonBox::RejectRole:
            mSettings.revertAll();
            break;

        case QDialogButtonBox::ResetRole:
        {
            int answer = QMessageBox::question(this, tr("Restore default settings"),
                tr("Are you sure to reset all settings to their default values?\n"
                   "After applying you cannot undo this change."));
            if (answer == QMessageBox::Yes)
                mSettings.restoreDefaults();
            break;
        }

        default:
            qCritical() << "invalid button role:" << mUi->buttonBox->buttonRole(button);
            break;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
