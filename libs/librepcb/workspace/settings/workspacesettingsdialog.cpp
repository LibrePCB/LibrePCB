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
#include <QtWidgets>
#include "workspacesettingsdialog.h"
#include "ui_workspacesettingsdialog.h"
#include "../workspace.h"
#include "workspacesettings.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettingsDialog::WorkspaceSettingsDialog(WorkspaceSettings& settings) :
    QDialog(0), mUi(new Ui::WorkspaceSettingsDialog), mSettings(settings)
{
    mUi->setupUi(this);

    // Add all settings widgets

    // tab: general
    mUi->generalLayout->addRow(mSettings.getAppLocale().getLabelText(),
                               mSettings.getAppLocale().getWidget());
    mUi->generalLayout->addRow(mSettings.getAppDefMeasUnits().getLengthUnitLabelText(),
                               mSettings.getAppDefMeasUnits().getLengthUnitComboBox());
    mUi->generalLayout->addRow(mSettings.getProjectAutosaveInterval().getLabelText(),
                               mSettings.getProjectAutosaveInterval().getWidget());

    // tab: appearance
    mUi->appearanceLayout->addRow(mSettings.getAppearance().getUseOpenGlLabelText(),
                                  mSettings.getAppearance().getUseOpenGlWidget());

    // tab: library
    mUi->libraryLayout->addRow(mSettings.getLibLocaleOrder().getLabelText(),
                               mSettings.getLibLocaleOrder().getWidget());
    mUi->libraryLayout->addRow(mSettings.getLibNormOrder().getLabelText(),
                               mSettings.getLibNormOrder().getWidget());

    // tab: repositories
    mUi->repositoriesLayout->addWidget(mSettings.getRepositories().getWidget());

    // tab: debug tools
    mUi->tabWidget->addTab(mSettings.getDebugTools().getWidget(), tr("Debug Tools"));

    // load the window geometry
    QSettings clientSettings;
    restoreGeometry(clientSettings.value("workspace_settings_dialog/window_geometry").toByteArray());

    // just in case that the wrong tab is selected in the UI designer:
    mUi->tabWidget->setCurrentIndex(0);
}

WorkspaceSettingsDialog::~WorkspaceSettingsDialog()
{
    // save the window geometry
    QSettings clientSettings;
    clientSettings.setValue("workspace_settings_dialog/window_geometry", saveGeometry());

    // Remove all settings widgets from the dialog (important for memory management!)

    // tab: general
    mSettings.getAppLocale().getWidget()->setParent(0);
    mSettings.getAppDefMeasUnits().getLengthUnitComboBox()->setParent(0);
    mSettings.getProjectAutosaveInterval().getWidget()->setParent(0);

    // tab: appearance
    mSettings.getAppearance().getUseOpenGlWidget()->setParent(0);

    // tab: library
    mSettings.getLibLocaleOrder().getWidget()->setParent(0);
    mSettings.getLibNormOrder().getWidget()->setParent(0);

    // tab: repositories
    mSettings.getRepositories().getWidget()->setParent(0);

    // tab: debug tools
    mUi->tabWidget->removeTab(mUi->tabWidget->indexOf(mSettings.getDebugTools().getWidget()));
    mSettings.getDebugTools().getWidget()->setParent(0);

    // delete private member objects
    delete mUi;         mUi = 0;
}

/*****************************************************************************************
 *  Inherited from QDialog
 ****************************************************************************************/

void WorkspaceSettingsDialog::accept()
{
    try {
        mSettings.applyAll(); // can throw
        QDialog::accept();
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
        QDialog::reject();
    }
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
            try {
                mSettings.applyAll(); // can throw
            } catch (const Exception& e) {
                QMessageBox::critical(this, tr("Error"), e.getMsg());
            }
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

} // namespace workspace
} // namespace librepcb
