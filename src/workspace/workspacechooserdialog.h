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

#ifndef WORKSPACECHOOSERDIALOG_H
#define WORKSPACECHOOSERDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace Ui {
class WorkspaceChooserDialog;
}

/*****************************************************************************************
 *  Class WorkspaceChooserDialog
 ****************************************************************************************/

/**
 * @brief The WorkspaceChooserDialog class
 *
 * @author ubruhin
 *
 * @date 2014-06-23
 */
class WorkspaceChooserDialog : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WorkspaceChooserDialog();
        ~WorkspaceChooserDialog();

        // Getters
        const QDir& getChoosedWorkspaceDir() const {return mChoosedWorkspaceDir;}

    public slots:

        virtual void accept();
        virtual void reject();

    private slots:

        // UI slots
        void on_addExistingWorkspaceButton_clicked();
        void on_createNewWorkspaceButton_clicked();
        void on_removeWorkspaceButton_clicked();
        void on_workspacesListWidget_currentItemChanged(QListWidgetItem* current,
                                                        QListWidgetItem* previous);

    private:

        // make some methods inaccessible...
        WorkspaceChooserDialog(const WorkspaceChooserDialog& other);
        WorkspaceChooserDialog& operator=(const WorkspaceChooserDialog& rhs);

        // Private Methods
        void saveWorkspacePaths() const;

        Ui::WorkspaceChooserDialog* ui;

        QDir mChoosedWorkspaceDir;
};

#endif // WORKSPACECHOOSERDIALOG_H
