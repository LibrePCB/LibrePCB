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
#include "items/si_symbol.h"
#include "../../library/genericcomponent.h"
#include "../../library/symbol.h"
#include "../project.h"
#include "../../common/undostack.h"
#include "../../common/undocommand.h"
#include "../circuit/cmd/cmdgencompinstedit.h"
#include "cmd/cmdsymbolinstanceedit.h"
#include "../circuit/gencompattributeinstance.h"
#include "../circuit/cmd/cmdgencompattrinstadd.h"
#include "../circuit/cmd/cmdgencompattrinstremove.h"
#include "../circuit/cmd/cmdgencompattrinstedit.h"
#include "../../common/attributes/attributetype.h"
#include "../../common/attributes/attributeunit.h"
#include "../settings/projectsettings.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstancePropertiesDialog::SymbolInstancePropertiesDialog(Project& project,
                                                               GenCompInstance& genComp,
                                                               SI_Symbol& symbol,
                                                               QWidget* parent) noexcept :
    QDialog(parent), mProject(project), mGenCompInstance(genComp), mSymbol(symbol),
    mUi(new Ui::SymbolInstancePropertiesDialog), mCommandActive(false),
    mAttributesEdited(false), mSelectedAttrItem(nullptr), mSelectedAttrType(nullptr),
    mSelectedAttrUnit(nullptr)
{
    mUi->setupUi(this);
    setWindowTitle(QString(tr("Properties of %1")).arg(mSymbol.getName()));
    mUi->tblGenCompInstAttributes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    foreach (const AttributeType* type, AttributeType::getAllTypes())
        mUi->cbxAttrType->addItem(type->getNameTr(), type->getName());

    // Generic Component Instance Attributes
    mUi->lblGenCompInstUuid->setText(mGenCompInstance.getUuid().toString());
    mUi->edtGenCompInstName->setText(mGenCompInstance.getName());
    mUi->edtGenCompInstValue->setText(mGenCompInstance.getValue());
    foreach (GenCompAttributeInstance* attr, mGenCompInstance.getAttributes())
    {
        AttrItem_t* item = new AttrItem_t();
        item->key = attr->getKey();
        item->type = &attr->getType();
        item->value = attr->getValue();
        item->unit = attr->getUnit();
        mAttrItems.append(item);
    }
    updateAttrTable();

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

    // Generic Component Library Element Attributes
    mUi->lblGenCompLibUuid->setText(mGenCompInstance.getGenComp().getUuid().toString());
    mUi->lblGenCompLibName->setText(mGenCompInstance.getGenComp().getName(localeOrder));
    mUi->lblGenCompLibName->setToolTip(mGenCompInstance.getGenComp().getDescription(localeOrder));
    mUi->lblSymbVarUuid->setText(mGenCompInstance.getSymbolVariant().getUuid().toString());
    mUi->lblSymbVarName->setText(mGenCompInstance.getSymbolVariant().getName(localeOrder));
    mUi->lblSymbVarName->setToolTip(mGenCompInstance.getSymbolVariant().getDescription(localeOrder));

    // Symbol Instance Attributes
    mUi->lblSymbInstUuid->setText(mSymbol.getUuid().toString());
    mUi->lblSymbInstName->setText(mSymbol.getName());
    mUi->spbxSymbInstPosX->setValue(mSymbol.getPosition().getX().toMm());
    mUi->spbxSymbInstPosY->setValue(mSymbol.getPosition().getY().toMm());
    mUi->spbxSymbInstAngle->setValue(mSymbol.getAngle().toDeg());

    // Symbol Library Element Attributes
    mUi->lblSymbLibUuid->setText(mSymbol.getLibSymbol().getUuid().toString());
    mUi->lblSymbLibName->setText(mSymbol.getLibSymbol().getName(localeOrder));
    mUi->lblSymbLibName->setToolTip(mSymbol.getLibSymbol().getDescription(localeOrder));

    // set focus to component instance name
    mUi->edtGenCompInstName->selectAll();
    mUi->edtGenCompInstName->setFocus();
}

SymbolInstancePropertiesDialog::~SymbolInstancePropertiesDialog() noexcept
{
    qDeleteAll(mAttrItems);     mAttrItems.clear();
    delete mUi;                 mUi = nullptr;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void SymbolInstancePropertiesDialog::on_tblGenCompInstAttributes_currentCellChanged(
        int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);
    if ((currentRow != previousRow) && (currentRow > -1))
    {
        QTableWidgetItem* item = mUi->tblGenCompInstAttributes->item(currentRow, 0);
        Q_ASSERT(item);
        QString key = item->text();
        mSelectedAttrItem = nullptr;
        foreach (AttrItem_t* item, mAttrItems) if (item->key == key) mSelectedAttrItem = item;
        Q_ASSERT(mSelectedAttrItem); if (!mSelectedAttrItem) return;
        mUi->edtAttrKey->setText(mSelectedAttrItem->key);
        mUi->cbxAttrType->setCurrentText(mSelectedAttrItem->type->getNameTr());
        mUi->edtAttrValue->setText(mSelectedAttrItem->type->printableValueTr(mSelectedAttrItem->value));
        mUi->cbxAttrUnit->setCurrentText(mSelectedAttrItem->unit ? mSelectedAttrItem->unit->getSymbolTr() : "");
    }
    else if (currentRow < 0)
        mSelectedAttrItem = nullptr;
}

void SymbolInstancePropertiesDialog::on_cbxAttrType_currentIndexChanged(int index)
{
    mUi->cbxAttrUnit->clear();
    mSelectedAttrUnit = nullptr;
    mSelectedAttrType = nullptr;
    if (index > -1)
    {
        try
        {
            mSelectedAttrType = &AttributeType::fromString(mUi->cbxAttrType->itemData(index).toString());
            foreach (const AttributeUnit* unit, mSelectedAttrType->getAvailableUnits())
                mUi->cbxAttrUnit->addItem(unit->getSymbolTr(), unit->getName());
            mSelectedAttrUnit = mSelectedAttrType->getDefaultUnit();
            mUi->cbxAttrUnit->setCurrentIndex(mSelectedAttrType->getAvailableUnits().indexOf(mSelectedAttrUnit));
        }
        catch (Exception& e)
        {
        }
    }
}

void SymbolInstancePropertiesDialog::on_cbxAttrUnit_currentIndexChanged(int index)
{
    mSelectedAttrUnit = nullptr;
    if ((index > -1) && (mSelectedAttrType != nullptr))
    {
        try
        {
            QString unitName = mUi->cbxAttrUnit->itemData(index).toString();
            mSelectedAttrUnit = mSelectedAttrType->getUnitFromString(unitName);
        }
        catch (Exception& e)
        {
        }
    }
}

void SymbolInstancePropertiesDialog::on_btnAttrApply_clicked()
{
    if (!mSelectedAttrItem) return;
    Q_ASSERT(mAttrItems.contains(mSelectedAttrItem) == true);

    QString key = mUi->edtAttrKey->text().trimmed();
    QString type = mUi->cbxAttrType->currentData().toString();
    QString value = mUi->edtAttrValue->text();
    QString unit = mUi->cbxAttrUnit->currentData().toString();

    try
    {
        AttrItem_t* attrItem = nullptr;
        foreach (AttrItem_t* item, mAttrItems)
            if ((item->key == key) && (item != mSelectedAttrItem)) attrItem = item;

        if (attrItem)
        {
            throw RuntimeError(__FILE__, __LINE__, key,
                QString(tr("There is already an attribute with the key \"%1\".")).arg(key));
        }
        else
        {
            const AttributeType& newType = AttributeType::fromString(type);
            const AttributeUnit* newUnit = newType.getUnitFromString(unit);
            QString newValue = newType.valueFromTr(value);
            if (!value.isEmpty()) newType.throwIfValueInvalid(value);
            mSelectedAttrItem->key = key;
            mSelectedAttrItem->type = &newType;
            mSelectedAttrItem->value = newValue;
            mSelectedAttrItem->unit = newUnit;
            mAttributesEdited = true;
            updateAttrTable();
        }
    }
    catch (Exception& e)
    {
        QMessageBox::warning(this, tr("Error"), e.getUserMsg());
    }
}

void SymbolInstancePropertiesDialog::on_btnAttrAdd_clicked()
{
    QString key = mUi->edtAttrKey->text().trimmed();
    QString type = mUi->cbxAttrType->currentData().toString();
    QString value = mUi->edtAttrValue->text();
    QString unit = mUi->cbxAttrUnit->currentData().toString();

    try
    {
        AttrItem_t* attrItem = nullptr;
        foreach (AttrItem_t* item, mAttrItems) if (item->key == key) attrItem = item;

        if (attrItem)
        {
            throw RuntimeError(__FILE__, __LINE__, key,
                QString(tr("There is already an attribute with the key \"%1\".")).arg(key));
        }
        else
        {
            const AttributeType& newType = AttributeType::fromString(type);
            const AttributeUnit* newUnit = newType.getUnitFromString(unit);
            QString newValue = newType.valueFromTr(value);
            if (!value.isEmpty()) newType.throwIfValueInvalid(value);
            AttrItem_t* item = new AttrItem_t();
            item->key = key;
            item->type = &newType;
            item->value = newValue;
            item->unit = newUnit;
            mAttrItems.append(item);
            mAttributesEdited = true;
            updateAttrTable();
        }
    }
    catch (Exception& e)
    {
        QMessageBox::warning(this, tr("Error"), e.getUserMsg());
    }
}

void SymbolInstancePropertiesDialog::on_btnAttrRemove_clicked()
{
    if (!mSelectedAttrItem) return;
    Q_ASSERT(mAttrItems.contains(mSelectedAttrItem) == true);
    mAttrItems.removeOne(mSelectedAttrItem);
    delete mSelectedAttrItem;   mSelectedAttrItem = nullptr;
    mAttributesEdited = true;
    updateAttrTable();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void SymbolInstancePropertiesDialog::updateAttrTable() noexcept
{
    QModelIndex index = mUi->tblGenCompInstAttributes->currentIndex();
    mUi->tblGenCompInstAttributes->setRowCount(0);
    foreach (const AttrItem_t* item, mAttrItems)
    {
        int i = mUi->tblGenCompInstAttributes->rowCount();
        mUi->tblGenCompInstAttributes->insertRow(i);
        mUi->tblGenCompInstAttributes->setItem(i, 0, new QTableWidgetItem(item->key));
        mUi->tblGenCompInstAttributes->setItem(i, 1, new QTableWidgetItem(item->type->getNameTr()));
        mUi->tblGenCompInstAttributes->setItem(i, 2, new QTableWidgetItem(item->type->printableValueTr(item->value, item->unit)));
    }
    if ((index.isValid()) && (mUi->tblGenCompInstAttributes->item(index.row(), index.column())))
        mUi->tblGenCompInstAttributes->setCurrentIndex(index);
}

void SymbolInstancePropertiesDialog::keyPressEvent(QKeyEvent* e)
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
        // Generic Component Instance
        QString name = mUi->edtGenCompInstName->text();
        if (name != mGenCompInstance.getName())
        {
            auto cmd = new CmdGenCompInstEdit(mProject.getCircuit(), mGenCompInstance);
            cmd->setName(name);
            execCmd(cmd);
        }
        QString value = mUi->edtGenCompInstValue->toPlainText();
        if (value != mGenCompInstance.getValue())
        {
            auto cmd = new CmdGenCompInstEdit(mProject.getCircuit(), mGenCompInstance);
            cmd->setValue(value);
            execCmd(cmd);
        }
        if (mAttributesEdited)
        {
            foreach (const AttrItem_t* item, mAttrItems)
            {
                GenCompAttributeInstance* attr = mGenCompInstance.getAttributeByKey(item->key);
                if (attr)
                {
                    // edit attribute
                    auto cmd = new CmdGenCompAttrInstEdit(mGenCompInstance, *attr, *item->type,
                                                          item->value, item->unit);
                    execCmd(cmd);
                }
                else
                {
                    // add attribute
                    auto cmd = new CmdGenCompAttrInstAdd(mGenCompInstance, item->key,
                                                         *item->type, item->value, item->unit);
                    execCmd(cmd);
                }
            }
            foreach (GenCompAttributeInstance* inst, mGenCompInstance.getAttributes())
            {
                bool removed = true;
                foreach (const AttrItem_t* item, mAttrItems)
                    if (item->key == inst->getKey()) removed = false;
                if (removed)
                {
                    // remove attribute
                    auto cmd = new CmdGenCompAttrInstRemove(mGenCompInstance, *inst);
                    execCmd(cmd);
                }
            }
        }

        // Symbol Instance
        Point pos(Length::fromMm(mUi->spbxSymbInstPosX->value()),
                  Length::fromMm(mUi->spbxSymbInstPosY->value()));
        Angle rotation = Angle::fromDeg(mUi->spbxSymbInstAngle->value());
        if ((pos != mSymbol.getPosition()) || (rotation != mSymbol.getAngle()))
        {
            CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(mSymbol);
            cmd->setPosition(pos, false);
            cmd->setRotation(rotation, false);
            execCmd(cmd);
        }

        endCmd();
        mAttributesEdited = false;
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
                                             .arg(mSymbol.getName()));
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
