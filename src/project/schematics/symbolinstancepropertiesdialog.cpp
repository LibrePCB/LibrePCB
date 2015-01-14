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
#include "symbolinstancepropertiesdialog.h"
#include "ui_symbolinstancepropertiesdialog.h"
#include "../circuit/gencompinstance.h"
#include "symbolinstance.h"
#include "../../library/genericcomponent.h"
#include "../../library/symbol.h"
#include "../project.h"
#include "../../common/undostack.h"
#include "../../common/undocommand.h"
#include "../circuit/cmd/cmdgencompinstsetname.h"
#include "cmd/cmdsymbolinstancemove.h"
#include "../circuit/cmd/cmdgencompinstsetvalue.h"
#include "../circuit/gencompattributeinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstancePropertiesDialog::SymbolInstancePropertiesDialog(Project& project,
                                                               GenCompInstance& genComp,
                                                               SymbolInstance& symbol,
                                                               QWidget* parent) noexcept :
    QDialog(parent), mProject(project), mGenCompInstance(genComp), mSymbolInstance(symbol),
    mUi(new Ui::SymbolInstancePropertiesDialog), mCommandActive(false)
{
    mUi->setupUi(this);
    setWindowTitle(QString(tr("Properties of %1")).arg(mSymbolInstance.getName()));

    // Generic Component Instance Attributes
    mUi->lblGenCompInstUuid->setText(mGenCompInstance.getUuid().toString());
    mUi->edtGenCompInstName->setText(mGenCompInstance.getName());
    mUi->edtGenCompInstValue->setText(mGenCompInstance.getValue());
    foreach (GenCompAttributeInstance* attr, mGenCompInstance.getAttributes())
    {
        int index = mUi->tblGenCompInstAttributes->rowCount();
        mUi->tblGenCompInstAttributes->insertRow(index);
        mUi->tblGenCompInstAttributes->setItem(index, 0, new QTableWidgetItem(attr->getKey()));
        mUi->tblGenCompInstAttributes->setItem(index, 1, new QTableWidgetItem(library::Attribute::typeToString(attr->getType())));
        mUi->tblGenCompInstAttributes->setItem(index, 2, new QTableWidgetItem(attr->getValueToDisplay()));
    }

    // Generic Component Library Element Attributes
    mUi->lblGenCompLibUuid->setText(mGenCompInstance.getGenComp().getUuid().toString());
    mUi->lblGenCompLibName->setText(mGenCompInstance.getGenComp().getName());
    mUi->lblGenCompLibDesc->setText(mGenCompInstance.getGenComp().getDescription());
    mUi->lblSymbVarUuid->setText(mGenCompInstance.getSymbolVariant().getUuid().toString());
    mUi->lblSymbVarName->setText(mGenCompInstance.getSymbolVariant().getName());
    mUi->lblSymbVarDesc->setText(mGenCompInstance.getSymbolVariant().getDescription());

    // Symbol Instance Attributes
    mUi->lblSymbInstUuid->setText(mSymbolInstance.getUuid().toString());
    mUi->lblSymbInstName->setText(mSymbolInstance.getName());
    mUi->spbxSymbInstPosX->setValue(mSymbolInstance.getPosition().getX().toMm());
    mUi->spbxSymbInstPosY->setValue(mSymbolInstance.getPosition().getY().toMm());
    mUi->spbxSymbInstAngle->setValue(mSymbolInstance.getAngle().toDeg());

    // Symbol Library Element Attributes
    mUi->lblSymbLibUuid->setText(mSymbolInstance.getSymbol().getUuid().toString());
    mUi->lblSymbLibName->setText(mSymbolInstance.getSymbol().getName());
    mUi->lblSymbLibDesc->setText(mSymbolInstance.getSymbol().getDescription());

    // set focus to component instance name
    mUi->edtGenCompInstName->selectAll();
    mUi->edtGenCompInstName->setFocus();
}

SymbolInstancePropertiesDialog::~SymbolInstancePropertiesDialog() noexcept
{
    delete mUi;     mUi = nullptr;
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void SymbolInstancePropertiesDialog::accept()
{
    // apply all changes
    if (applyChanges())
        QDialog::accept();
}

bool SymbolInstancePropertiesDialog::applyChanges() noexcept
{
    try
    {
        // Generic Component Instance Attributes
        QString name = mUi->edtGenCompInstName->text();
        if (name != mGenCompInstance.getName())
        {
            CmdGenCompInstSetName* cmd = new CmdGenCompInstSetName(mProject.getCircuit(),
                                                                   mGenCompInstance, name);
            execCmd(cmd);
        }
        QString value = mUi->edtGenCompInstValue->toPlainText();
        if (value != mGenCompInstance.getValue())
        {
            CmdGenCompInstSetValue* cmd = new CmdGenCompInstSetValue(mGenCompInstance, value);
            execCmd(cmd);
        }


        // Symbol Instance Attributes
        Length x = Length::fromMm(mUi->spbxSymbInstPosX->value());
        Length y = Length::fromMm(mUi->spbxSymbInstPosY->value());
        Point pos(x, y);
        if (pos != mSymbolInstance.getPosition())
        {
            CmdSymbolInstanceMove* cmd = new CmdSymbolInstanceMove(mSymbolInstance);
            cmd->setAbsolutePosTemporary(pos);
            execCmd(cmd);
        }
        Angle angle = Angle::fromDeg(mUi->spbxSymbInstAngle->value());
        if (angle != mSymbolInstance.getAngle())
        {
            CmdSymbolInstanceMove* cmd = new CmdSymbolInstanceMove(mSymbolInstance);
            cmd->setAngleTemporary(angle);
            execCmd(cmd);
        }

        endCmd();
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        try {abortCmd();} catch (...) {}
        return false;
    }
}

void SymbolInstancePropertiesDialog::execCmd(UndoCommand* cmd)
{
    if (!mCommandActive)
    {
        mProject.getUndoStack().beginCommand(QString(tr("Change properties of %1"))
                                             .arg(mSymbolInstance.getName()));
        mCommandActive = true;
    }

    mProject.getUndoStack().appendToCommand(cmd);
}

void SymbolInstancePropertiesDialog::endCmd()
{
    if (mCommandActive)
    {
        mProject.getUndoStack().endCommand();
        mCommandActive = false;
    }
}

void SymbolInstancePropertiesDialog::abortCmd()
{
    if (mCommandActive)
    {
        mProject.getUndoStack().abortCommand();
        mCommandActive = false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
