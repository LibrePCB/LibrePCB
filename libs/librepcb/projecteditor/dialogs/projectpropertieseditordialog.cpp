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
#include "projectpropertieseditordialog.h"
#include "ui_projectpropertieseditordialog.h"
#include <librepcb/project/project.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/cmd/cmdprojectsetmetadata.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectPropertiesEditorDialog::ProjectPropertiesEditorDialog(Project& project,
        UndoStack& undoStack, QWidget* parent) noexcept :
    QDialog(parent), mProject(project), mUndoStack(undoStack),
    mUi(new Ui::ProjectPropertiesEditorDialog)
{
    mUi->setupUi(this);

    mUi->edtName->setText(mProject.getName());
    mUi->edtAuthor->setText(mProject.getAuthor());
    mUi->edtVersion->setText(mProject.getVersion());
    mUi->lblCreatedDateTime->setText(mProject.getCreated().toString(Qt::DefaultLocaleLongDate));
    mUi->lblLastModifiedDateTime->setText(mProject.getLastModified().toString(Qt::DefaultLocaleLongDate));
    mUi->attributeListEditorWidget->setAttributeList(mProject.getAttributes());
}

ProjectPropertiesEditorDialog::~ProjectPropertiesEditorDialog() noexcept
{
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
    if (applyChanges()) {
        QDialog::accept();
    }
}

bool ProjectPropertiesEditorDialog::applyChanges() noexcept
{
    try {
        CmdProjectSetMetadata* cmd = new CmdProjectSetMetadata(mProject);
        cmd->setName(mUi->edtName->text().trimmed());
        cmd->setAuthor(mUi->edtAuthor->text().trimmed());
        cmd->setVersion(mUi->edtVersion->text().trimmed());
        cmd->setAttributes(mUi->attributeListEditorWidget->getAttributeList());
        mUndoStack.execCmd(cmd);
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
