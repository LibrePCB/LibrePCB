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
#include "workspacechooserdialog.h"
#include "ui_workspacechooserdialog.h"
#include "workspace.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceChooserDialog::WorkspaceChooserDialog() :
    QDialog(0), ui(new Ui::WorkspaceChooserDialog)
{
    ui->setupUi(this);

    mChoosedWorkspaceDir.setPath(""); // clear the default path

    foreach (QString path, Workspace::getAllWorkspacePaths())
         ui->workspacesListWidget->addItem(path);
}

WorkspaceChooserDialog::~WorkspaceChooserDialog()
{
    delete ui;          ui = 0;
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WorkspaceChooserDialog::accept()
{
    if ((ui->workspacesListWidget->selectedItems().count() == 1)
        && (Workspace::isValidWorkspaceDir(mChoosedWorkspaceDir)))
    {
        saveWorkspacePaths();
        Workspace::setMostRecentlyUsedWorkspacePath(mChoosedWorkspaceDir.path());
        QDialog::accept();
    }
}

void WorkspaceChooserDialog::reject()
{
    mChoosedWorkspaceDir.setPath(""); // make the path invalid
    saveWorkspacePaths();
    QDialog::reject();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void WorkspaceChooserDialog::on_addExistingWorkspaceButton_clicked()
{
    QDir dir;
    dir.setPath(QFileDialog::getExistingDirectory(0, "Select Workspace Path", ""));

    if (dir.absolutePath().isEmpty())
        return;

    if ((!ui->workspacesListWidget->findItems(dir.path(), Qt::MatchExactly).isEmpty()) ||
            (!ui->workspacesListWidget->findItems(dir.absolutePath(), Qt::MatchExactly).isEmpty()))
    {
        QMessageBox::warning(this, "Warning", "This workspace is already in the list!");
        return;
    }

    if (!Workspace::isValidWorkspaceDir(dir))
    {
        QMessageBox::warning(this, "Warning", "No workspace found in the selected directory!");
        return;
    }

    ui->workspacesListWidget->addItem(dir.path());
}

void WorkspaceChooserDialog::on_createNewWorkspaceButton_clicked()
{
    QDir dir;
    dir.setPath(QFileDialog::getExistingDirectory(0, "Select Workspace Path", ""));

    if (dir.absolutePath().isEmpty())
        return;

    if ((!ui->workspacesListWidget->findItems(dir.path(), Qt::MatchExactly).isEmpty()) ||
            (!ui->workspacesListWidget->findItems(dir.absolutePath(), Qt::MatchExactly).isEmpty()))
    {
        QMessageBox::warning(this, "Warning", "This workspace is already in the list!");
        return;
    }

    try
    {
        Workspace::createNewWorkspace(dir);
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }

    ui->workspacesListWidget->addItem(dir.path());
}

void WorkspaceChooserDialog::on_removeWorkspaceButton_clicked()
{
    qDeleteAll(ui->workspacesListWidget->selectedItems());
}

void WorkspaceChooserDialog::on_workspacesListWidget_currentItemChanged(QListWidgetItem* current,
                                                                        QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    QString path = (current == 0) ? "" : current->text();
    mChoosedWorkspaceDir.setPath(path);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WorkspaceChooserDialog::saveWorkspacePaths() const
{
    QStringList paths;
    for (int i = 0; i < ui->workspacesListWidget->count(); i++)
         paths.append(ui->workspacesListWidget->item(i)->text());

    Workspace::setAllWorkspacePaths(paths);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
