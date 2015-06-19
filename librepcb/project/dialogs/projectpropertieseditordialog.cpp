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
#include "projectpropertieseditordialog.h"
#include "ui_projectpropertieseditordialog.h"
#include "../project.h"
#include <eda4ucommon/undostack.h>
#include "../cmd/cmdprojectsetmetadata.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectPropertiesEditorDialog::ProjectPropertiesEditorDialog(Project& project, QWidget* parent) :
    QDialog(parent), mProject(project), mUi(new Ui::ProjectPropertiesEditorDialog),
    mCommandActive(false)
{
    mUi->setupUi(this);

    mUi->edtName->setText(mProject.getName());
    mUi->edtDescription->setPlainText(mProject.getDescription());
    mUi->edtAuthor->setText(mProject.getAuthor());
    mUi->edtCreated->setDateTime(mProject.getCreated());
}

ProjectPropertiesEditorDialog::~ProjectPropertiesEditorDialog()
{
    delete mUi;     mUi = nullptr;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ProjectPropertiesEditorDialog::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
        case Qt::Key_Return:
            accept();
            break;
        case Qt::Key_Escape:
            reject();
            break;
        default:
            QDialog::keyPressEvent(e);
            break;
    }
}

void ProjectPropertiesEditorDialog::accept()
{
    // apply all changes
    if (applyChanges())
        QDialog::accept();
}

bool ProjectPropertiesEditorDialog::applyChanges() noexcept
{
    try
    {
        mProject.getUndoStack().beginCommand(tr("Change project properties"));
        mCommandActive = true;

        // Metadata
        CmdProjectSetMetadata* cmd = new CmdProjectSetMetadata(mProject);
        cmd->setName(mUi->edtName->text());
        cmd->setDescription(mUi->edtDescription->toPlainText());
        cmd->setAuthor(mUi->edtAuthor->text());
        cmd->setCreated(mUi->edtCreated->dateTime());
        mProject.getUndoStack().appendToCommand(cmd);

        mProject.getUndoStack().endCommand();
        mCommandActive = false;
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        try {if (mCommandActive) mProject.getUndoStack().abortCommand();} catch (...) {}
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
