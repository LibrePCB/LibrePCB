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
#include "boardviapropertiesdialog.h"
#include "ui_boardviapropertiesdialog.h"
#include <librepcb/common/undostack.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardViaPropertiesDialog::BoardViaPropertiesDialog(Project& project, BI_Via& via,
                                                   UndoStack& undoStack, QWidget *parent) noexcept :
    QDialog(parent), mProject(project), mVia(via), mUi(new Ui::BoardViaPropertiesDialog),
    mUndoStack(undoStack)
{
    mUi->setupUi(this);

    // UUID label
    mUi->lblUuid->setText(mVia.getUuid().toStr());

    // shape combobox
    mUi->cbxShape->addItem(tr("Round"), static_cast<int>(BI_Via::Shape::Round));
    mUi->cbxShape->addItem(tr("Square"), static_cast<int>(BI_Via::Shape::Square));
    mUi->cbxShape->addItem(tr("Octagon"), static_cast<int>(BI_Via::Shape::Octagon));
    mUi->cbxShape->setCurrentIndex(mUi->cbxShape->findData(static_cast<int>(mVia.getShape())));

    // Position spinboxes
    mUi->spbxPosX->setValue(mVia.getPosition().getX().toMm());
    mUi->spbxPosY->setValue(mVia.getPosition().getY().toMm());

    // size spinbox
    mUi->spbxSize->setValue(mVia.getSize().toMm());

    // drill diameter spinbox
    mUi->spbxDrillDiameter->setValue(mVia.getDrillDiameter().toMm());

    // netsignal combobox
    mUi->cbxNetSignal->addItem("");
    foreach (NetSignal* netsignal, mProject.getCircuit().getNetSignals()) {
        mUi->cbxNetSignal->addItem(netsignal->getName());
    }
    QString netsignal = mVia.getNetSignal() ? mVia.getNetSignal()->getName() : "";
    mUi->cbxNetSignal->setCurrentText(netsignal);
    mUi->cbxNetSignal->model()->sort(0);
}

BoardViaPropertiesDialog::~BoardViaPropertiesDialog() noexcept
{
    mUi.reset();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardViaPropertiesDialog::keyPressEvent(QKeyEvent* e)
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

void BoardViaPropertiesDialog::accept()
{
    if (applyChanges()) {
        QDialog::accept();
    }
}

bool BoardViaPropertiesDialog::applyChanges() noexcept
{
    try
    {
        QString netsignalStr = mUi->cbxNetSignal->currentText();
        NetSignal* netsignal = mProject.getCircuit().getNetSignalByName(netsignalStr);
        if ((!netsignal) && (!netsignalStr.isEmpty())) {
            throw RuntimeError(__FILE__, __LINE__, QString(), tr("Invalid net signal."));
        }
        QScopedPointer<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(mVia));
        cmd->setShape(static_cast<BI_Via::Shape>(mUi->cbxShape->currentData().toInt()), false);
        cmd->setPosition(Point(Length::fromMm(mUi->spbxPosX->value()),
                               Length::fromMm(mUi->spbxPosY->value())), false);
        cmd->setSize(Length::fromMm(mUi->spbxSize->value()), false);
        cmd->setDrillDiameter(Length::fromMm(mUi->spbxDrillDiameter->value()), false);
        cmd->setNetSignal(netsignal, false); // can throw
        mUndoStack.execCmd(cmd.take());
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
