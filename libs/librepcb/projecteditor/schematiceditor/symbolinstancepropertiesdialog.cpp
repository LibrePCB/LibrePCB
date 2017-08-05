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
#include "symbolinstancepropertiesdialog.h"
#include "ui_symbolinstancepropertiesdialog.h"
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/project.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/undocommand.h>
#include <librepcb/project/circuit/cmd/cmdcomponentinstanceedit.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcb/common/attributes/attributetype.h>
#include <librepcb/common/attributes/attributeunit.h>
#include <librepcb/project/settings/projectsettings.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolInstancePropertiesDialog::SymbolInstancePropertiesDialog(Project& project,
        ComponentInstance& cmp, SI_Symbol& symbol, UndoStack& undoStack, QWidget* parent) noexcept :
    QDialog(parent), mProject(project), mComponentInstance(cmp), mSymbol(symbol),
    mUndoStack(undoStack), mUi(new Ui::SymbolInstancePropertiesDialog)
{
    mUi->setupUi(this);
    setWindowTitle(QString(tr("Properties of %1")).arg(mSymbol.getName()));

    // Component Instance Attributes
    mUi->lblCompInstUuid->setText(mComponentInstance.getUuid().toStr());
    mUi->edtCompInstName->setText(mComponentInstance.getName());
    mUi->edtCompInstValue->setText(mComponentInstance.getValue());
    mUi->attributeListEditorWidget->setAttributeList(mComponentInstance.getAttributes());

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

    // Component Library Element Attributes
    QString htmlLink("<a href=\"%1\">%2<a>");
    mUi->lblCompLibUuid->setText(htmlLink.arg(mComponentInstance.getLibComponent().getFilePath().toQUrl().toString(),
                                              mComponentInstance.getLibComponent().getUuid().toStr()));
    mUi->lblCompLibUuid->setToolTip(mComponentInstance.getLibComponent().getFilePath().toNative());
    mUi->lblCompLibName->setText(mComponentInstance.getLibComponent().getNames().value(localeOrder));
    mUi->lblCompLibName->setToolTip(mComponentInstance.getLibComponent().getDescriptions().value(localeOrder));
    mUi->lblSymbVarUuid->setText(mComponentInstance.getSymbolVariant().getUuid().toStr());
    mUi->lblSymbVarName->setText(mComponentInstance.getSymbolVariant().getNames().value(localeOrder));
    mUi->lblSymbVarName->setToolTip(mComponentInstance.getSymbolVariant().getDescriptions().value(localeOrder));

    // Symbol Instance Attributes
    mUi->lblSymbInstUuid->setText(mSymbol.getUuid().toStr());
    mUi->lblSymbInstName->setText(mSymbol.getName());
    mUi->spbxSymbInstPosX->setValue(mSymbol.getPosition().getX().toMm());
    mUi->spbxSymbInstPosY->setValue(mSymbol.getPosition().getY().toMm());
    mUi->spbxSymbInstAngle->setValue(mSymbol.getRotation().toDeg());

    // Symbol Library Element Attributes
    mUi->lblSymbLibUuid->setText(htmlLink.arg(mSymbol.getLibSymbol().getFilePath().toQUrl().toString(),
                                              mSymbol.getLibSymbol().getUuid().toStr()));
    mUi->lblSymbLibUuid->setToolTip(mSymbol.getLibSymbol().getFilePath().toNative());
    mUi->lblSymbLibName->setText(mSymbol.getLibSymbol().getNames().value(localeOrder));
    mUi->lblSymbLibName->setToolTip(mSymbol.getLibSymbol().getDescriptions().value(localeOrder));

    // set focus to component instance name
    mUi->edtCompInstName->selectAll();
    mUi->edtCompInstName->setFocus();
}

SymbolInstancePropertiesDialog::~SymbolInstancePropertiesDialog() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

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
    if (applyChanges()) {
        QDialog::accept();
    }
}

bool SymbolInstancePropertiesDialog::applyChanges() noexcept
{
    try {
        UndoStackTransaction transaction(mUndoStack,
            QString(tr("Change properties of %1")).arg(mSymbol.getName()));

        // Component Instance
        QScopedPointer<CmdComponentInstanceEdit> cmdCmp(
            new CmdComponentInstanceEdit(mProject.getCircuit(), mComponentInstance));
        cmdCmp->setName(mUi->edtCompInstName->text().trimmed());
        cmdCmp->setValue(mUi->edtCompInstValue->toPlainText());
        cmdCmp->setAttributes(mUi->attributeListEditorWidget->getAttributeList());
        transaction.append(cmdCmp.take());

        // Symbol Instance
        Point pos(Length::fromMm(mUi->spbxSymbInstPosX->value()),
                  Length::fromMm(mUi->spbxSymbInstPosY->value()));
        Angle rotation = Angle::fromDeg(mUi->spbxSymbInstAngle->value());
        QScopedPointer<CmdSymbolInstanceEdit> cmdSym(new CmdSymbolInstanceEdit(mSymbol));
        cmdSym->setPosition(pos, false);
        cmdSym->setRotation(rotation, false);
        transaction.append(cmdSym.take());

        transaction.commit(); // can throw
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
