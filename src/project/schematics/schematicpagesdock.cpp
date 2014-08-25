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
#include "schematicpagesdock.h"
#include "ui_schematicpagesdock.h"
#include "../project.h"
#include "schematic.h"
#include "cmd/cmdschematicadd.h"
#include "cmd/cmdschematicremove.h"
#include "../../common/undostack.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicPagesDock::SchematicPagesDock(Project& project) :
    QDockWidget(0), mProject(project), mUi(new Ui::SchematicPagesDock)
{
    mUi->setupUi(this);

    // add all schematics to list widget
    for (int i = 0; i < mProject.getSchematicCount(); i++)
        schematicAdded(i);

    // connect signals/slots
    connect(&mProject, SIGNAL(schematicAdded(int)), this, SLOT(schematicAdded(int)));
    connect(&mProject, SIGNAL(schematicRemoved(int)), this, SLOT(schematicRemoved(int)));
}

SchematicPagesDock::~SchematicPagesDock()
{
    delete mUi;         mUi = 0;
}

/*****************************************************************************************
 *  Inherited from QDockWidget
 ****************************************************************************************/

/**
 * @todo The width of the icons is not very accurate
 */
void SchematicPagesDock::resizeEvent(QResizeEvent* event)
{
    int iconSize = event->size().width(); // this is not good...
    mUi->listWidget->setIconSize(QSize(iconSize, iconSize));
    QDockWidget::resizeEvent(event);
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void SchematicPagesDock::schematicAdded(int newIndex)
{
    Schematic* schematic = mProject.getSchematicByIndex(newIndex);
    if (!schematic)
    {
        qCritical() << "schematic is NULL!";
        return;
    }

    QListWidgetItem* item = new QListWidgetItem();
    item->setText(schematic->getName());
    item->setIcon(schematic->getIcon());
    item->setData(Qt::UserRole, qVariantFromValue(schematic));
    mUi->listWidget->insertItem(newIndex, item);
}

void SchematicPagesDock::schematicRemoved(int oldIndex)
{
    delete mUi->listWidget->item(oldIndex);
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void SchematicPagesDock::on_btnNewSchematic_clicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Add schematic page"),
                       tr("Choose a name:"), QLineEdit::Normal, tr("New Page"), &ok);

    if (!ok)
        return;

    try
    {
        CmdSchematicAdd* cmd = new CmdSchematicAdd(mProject, name);
        mProject.getUndoStack().execCmd(cmd);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void SchematicPagesDock::on_btnRemoveSchematic_clicked()
{
    Schematic* schematic = mProject.getSchematicByIndex(mUi->listWidget->currentRow());
    if (!schematic)
        return;

    try
    {
        CmdSchematicRemove* cmd = new CmdSchematicRemove(mProject, schematic);
        mProject.getUndoStack().execCmd(cmd);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
