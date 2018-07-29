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
#include "symbolpinpropertiesdialog.h"
#include "ui_symbolpinpropertiesdialog.h"
#include <librepcb/common/undostack.h>
#include <librepcb/library/sym/symbolpin.h>
#include <librepcb/library/sym/cmd/cmdsymbolpinedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

SymbolPinPropertiesDialog::SymbolPinPropertiesDialog(SymbolPin& pin, UndoStack& undoStack,
                                                     QWidget* parent) noexcept :
    QDialog(parent), mSymbolPin(pin), mUndoStack(undoStack),
    mUi(new Ui::SymbolPinPropertiesDialog)
{
    mUi->setupUi(this);
    connect(mUi->buttonBox, &QDialogButtonBox::clicked,
            this, &SymbolPinPropertiesDialog::on_buttonBox_clicked);

    // load pin attributes
    mUi->lblUuid->setText(mSymbolPin.getUuid().toStr());
    mUi->edtName->setText(*mSymbolPin.getName());
    mUi->spbPosX->setValue(mSymbolPin.getPosition().getX().toMm());
    mUi->spbPosY->setValue(mSymbolPin.getPosition().getY().toMm());
    mUi->spbRotation->setValue(mSymbolPin.getRotation().toDeg());
    mUi->spbLength->setValue(mSymbolPin.getLength()->toMm());
}

SymbolPinPropertiesDialog::~SymbolPinPropertiesDialog() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void SymbolPinPropertiesDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (mUi->buttonBox->buttonRole(button)) {
        case QDialogButtonBox::ApplyRole:
            applyChanges();
            break;
        case QDialogButtonBox::AcceptRole:
            if (applyChanges()) {
                accept();
            }
            break;
        case QDialogButtonBox::RejectRole:
            reject();
            break;
        default: Q_ASSERT(false); break;
    }
}

bool SymbolPinPropertiesDialog::applyChanges() noexcept
{
    try {
        CircuitIdentifier name(mUi->edtName->text().trimmed()); // can throw
        QScopedPointer<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(mSymbolPin));
        cmd->setName(name, false);
        cmd->setLength(UnsignedLength(Length::fromMm(mUi->spbLength->value())), false); // can throw
        cmd->setPosition(Point::fromMm(mUi->spbPosX->value(), mUi->spbPosY->value()), false);
        cmd->setRotation(Angle::fromDeg(mUi->spbRotation->value()), false);
        mUndoStack.execCmd(cmd.take());
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
} // namespace library
} // namespace librepcb
