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
#include "projectsettingsdialog.h"
#include "ui_projectsettingsdialog.h"
#include "cmd/cmdprojectsettingschange.h"
#include "projectsettings.h"
#include "../project.h"
#include "../../common/undostack.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectSettingsDialog::ProjectSettingsDialog(ProjectSettings& settings, QWidget* parent) noexcept :
    QDialog(parent), mSettings(settings), mUi(new Ui::ProjectSettingsDialog)
{
    mUi->setupUi(this);
    mUi->tabWidget->setCurrentIndex(0);

    // list locales
    QStringList allLocales;
    allLocales << "en_US" << "en_GB" << "de_DE" << "de_CH" << "gsw_CH"; // TODO: add more locales
    allLocales.sort();
    foreach (const QString& localeStr, allLocales)
    {
        QLocale locale(localeStr);
        QString str = QString("[%1] %2 (%3)").arg(locale.name(), locale.nativeLanguageName(), locale.nativeCountryName());
        if (mUi->cbxLocales->findData(locale.name()) < 0)
            mUi->cbxLocales->addItem(str, locale.name());
    }
    mUi->cbxLocales->setCurrentIndex(mUi->cbxLocales->findData(QLocale().name()));

    // list norms
    mUi->cbxNorms->addItem("DIN EN 81346"); // TODO: add more norms (dynamically?)

    // update GUI elements
    updateGuiFromSettings();
}

ProjectSettingsDialog::~ProjectSettingsDialog() noexcept
{
    delete mUi;         mUi = nullptr;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void ProjectSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (mUi->buttonBox->buttonRole(button))
    {
        case QDialogButtonBox::AcceptRole:
        case QDialogButtonBox::RejectRole:
            // applySettings() will be called from accept()
            break;

        case QDialogButtonBox::ApplyRole:
            applySettings();
            break;

        case QDialogButtonBox::ResetRole:
        {
            int answer = QMessageBox::question(this, tr("Restore default settings"),
                tr("Are you sure to reset all settings to their default values?"));
            if (answer == QMessageBox::Yes)
                restoreDefaultSettings();
            break;
        }

        default:
            qCritical() << "invalid button role:" << mUi->buttonBox->buttonRole(button);
            break;
    }
}

void ProjectSettingsDialog::accept()
{
    if (applySettings())
        QDialog::accept();
}

void ProjectSettingsDialog::reject()
{
    QDialog::reject();
}

void ProjectSettingsDialog::on_btnLocaleAdd_clicked()
{
    if (mUi->cbxLocales->currentIndex() >= 0)
    {
        QString localeStr = mUi->cbxLocales->currentData().toString();
        QLocale locale(localeStr);
        QString str = QString("[%1] %2 (%3)").arg(locale.name(), locale.nativeLanguageName(), locale.nativeCountryName());
        QListWidgetItem* item = new QListWidgetItem(str, mUi->lstLocaleOrder);
        item->setData(Qt::UserRole, localeStr);
    }
}

void ProjectSettingsDialog::on_btnLocaleRemove_clicked()
{
    if (mUi->lstLocaleOrder->currentRow() >= 0)
        delete mUi->lstLocaleOrder->item(mUi->lstLocaleOrder->currentRow());
}

void ProjectSettingsDialog::on_btnLocaleUp_clicked()
{
    int row = mUi->lstLocaleOrder->currentRow();
    if (row > 0)
    {
        mUi->lstLocaleOrder->insertItem(row - 1, mUi->lstLocaleOrder->takeItem(row));
        mUi->lstLocaleOrder->setCurrentRow(row - 1);
    }
}

void ProjectSettingsDialog::on_btnLocaleDown_clicked()
{
    int row = mUi->lstLocaleOrder->currentRow();
    if ((row >= 0) && (row < mUi->lstLocaleOrder->count() - 1))
    {
        mUi->lstLocaleOrder->insertItem(row + 1, mUi->lstLocaleOrder->takeItem(row));
        mUi->lstLocaleOrder->setCurrentRow(row + 1);
    }
}

void ProjectSettingsDialog::on_btnNormAdd_clicked()
{
    if (!mUi->cbxNorms->currentText().isEmpty())
        mUi->lstNormOrder->addItem(mUi->cbxNorms->currentText());
}

void ProjectSettingsDialog::on_btnNormRemove_clicked()
{
    if (mUi->lstNormOrder->currentRow() >= 0)
        delete mUi->lstNormOrder->item(mUi->lstNormOrder->currentRow());
}

void ProjectSettingsDialog::on_btnNormUp_clicked()
{
    int row = mUi->lstNormOrder->currentRow();
    if (row > 0)
    {
        mUi->lstNormOrder->insertItem(row - 1, mUi->lstNormOrder->takeItem(row));
        mUi->lstNormOrder->setCurrentRow(row - 1);
    }
}

void ProjectSettingsDialog::on_btnNormDown_clicked()
{
    int row = mUi->lstNormOrder->currentRow();
    if ((row >= 0) && (row < mUi->lstNormOrder->count() - 1))
    {
        mUi->lstNormOrder->insertItem(row + 1, mUi->lstNormOrder->takeItem(row));
        mUi->lstNormOrder->setCurrentRow(row + 1);
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ProjectSettingsDialog::applySettings() noexcept
{
    try
    {
        CmdProjectSettingsChange* cmd = new CmdProjectSettingsChange(mSettings);

        // locales
        QStringList locales;
        for (int i=0; i<mUi->lstLocaleOrder->count(); i++)
            locales.append(mUi->lstLocaleOrder->item(i)->data(Qt::UserRole).toString());
        cmd->setLocaleOrder(locales);

        // norms
        QStringList norms;
        for (int i=0; i<mUi->lstNormOrder->count(); i++)
            norms.append(mUi->lstNormOrder->item(i)->text());
        cmd->setNormOrder(norms);

        // execute cmd
        mSettings.getProject().getUndoStack().execCmd(cmd);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        return false;
    }
}

bool ProjectSettingsDialog::restoreDefaultSettings() noexcept
{
    try
    {
        CmdProjectSettingsChange* cmd = new CmdProjectSettingsChange(mSettings);
        cmd->restoreDefaults();
        mSettings.getProject().getUndoStack().execCmd(cmd);
        updateGuiFromSettings();
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        return false;
    }
}

void ProjectSettingsDialog::updateGuiFromSettings() noexcept
{
    // locales
    mUi->lstLocaleOrder->clear();
    foreach (const QString& localeStr, mSettings.getLocaleOrder())
    {
        QLocale locale(localeStr);
        QString str = QString("[%1] %2 (%3)").arg(locale.name(), locale.nativeLanguageName(), locale.nativeCountryName());
        QListWidgetItem* item = new QListWidgetItem(str, mUi->lstLocaleOrder);
        item->setData(Qt::UserRole, localeStr);
    }

    // norms
    mUi->lstNormOrder->clear();
    mUi->lstNormOrder->addItems(mSettings.getNormOrder());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
