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
#include "footprintpadpropertiesdialog.h"
#include "ui_footprintpadpropertiesdialog.h"
#include <librepcb/common/undostack.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/library/pkg/cmd/cmdfootprintpadedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

FootprintPadPropertiesDialog::FootprintPadPropertiesDialog(const Package& pkg,
        const Footprint& fpt, FootprintPad& pad, UndoStack& undoStack, QWidget* parent) noexcept :
    QDialog(parent), mPad(pad), mUndoStack(undoStack),
    mUi(new Ui::FootprintPadPropertiesDialog)
{
    mUi->setupUi(this);
    connect(mUi->buttonBox, &QDialogButtonBox::clicked,
            this, &FootprintPadPropertiesDialog::on_buttonBox_clicked);

    // load pad attributes
    int currentPadIndex = 0;
    mUi->cbxPackagePad->addItem(tr("(not connected)"), "");
    for (const PackagePad& p : pkg.getPads()) {
        if ((p.getUuid() == mPad.getUuid()) || (!fpt.getPads().contains(p.getUuid()))) {
            mUi->cbxPackagePad->addItem(*p.getName(), p.getUuid().toStr());
            if (mPad.getPackagePadUuid() == p.getUuid()) {
                currentPadIndex = mUi->cbxPackagePad->count() - 1;
            }
        }
    }
    mUi->cbxPackagePad->setCurrentIndex(currentPadIndex);
    switch (mPad.getBoardSide()) {
        case FootprintPad::BoardSide::TOP:      mUi->rbtnBoardSideTop->setChecked(true); break;
        case FootprintPad::BoardSide::BOTTOM:   mUi->rbtnBoardSideBottom->setChecked(true); break;
        case FootprintPad::BoardSide::THT:      mUi->rbtnBoardSideTht->setChecked(true); break;
        default: Q_ASSERT(false); break;
    }
    switch (mPad.getShape()) {
        case FootprintPad::Shape::ROUND:    mUi->rbtnShapeRound->setChecked(true); break;
        case FootprintPad::Shape::RECT:     mUi->rbtnShapeRect->setChecked(true); break;
        case FootprintPad::Shape::OCTAGON:  mUi->rbtnShapeOctagon->setChecked(true); break;
        default: Q_ASSERT(false); break;
    }
    mUi->spbWidth->setValue(mPad.getWidth()->toMm());
    mUi->spbHeight->setValue(mPad.getHeight()->toMm());
    mUi->spbDrillDiameter->setValue(mPad.getDrillDiameter()->toMm());
    mUi->spbPosX->setValue(mPad.getPosition().getX().toMm());
    mUi->spbPosY->setValue(mPad.getPosition().getY().toMm());
    mUi->spbRotation->setValue(mPad.getRotation().toDeg());

    // disable drill diameter for SMT pads
    mUi->spbDrillDiameter->setEnabled(mUi->rbtnBoardSideTht->isChecked());
    connect(mUi->rbtnBoardSideTht, &QRadioButton::toggled,
            mUi->spbDrillDiameter, &QDoubleSpinBox::setEnabled);
}

FootprintPadPropertiesDialog::~FootprintPadPropertiesDialog() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void FootprintPadPropertiesDialog::on_buttonBox_clicked(QAbstractButton *button)
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

bool FootprintPadPropertiesDialog::applyChanges() noexcept
{
    try {
        QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(mPad));
        Uuid pkgPad = Uuid::fromString(mUi->cbxPackagePad->currentData().toString()); // can throw
        cmd->setPackagePadUuid(pkgPad, false);
        if (mUi->rbtnBoardSideTop->isChecked()) {
            cmd->setBoardSide(FootprintPad::BoardSide::TOP, false);
        } else if (mUi->rbtnBoardSideBottom->isChecked()) {
            cmd->setBoardSide(FootprintPad::BoardSide::BOTTOM, false);
        } else if (mUi->rbtnBoardSideTht->isChecked()) {
            cmd->setBoardSide(FootprintPad::BoardSide::THT, false);
        } else {
            Q_ASSERT(false);
        }
        if (mUi->rbtnShapeRound->isChecked()) {
            cmd->setShape(FootprintPad::Shape::ROUND, false);
        } else if (mUi->rbtnShapeRect->isChecked()) {
            cmd->setShape(FootprintPad::Shape::RECT, false);
        } else if (mUi->rbtnShapeOctagon->isChecked()) {
            cmd->setShape(FootprintPad::Shape::OCTAGON, false);
        } else {
            Q_ASSERT(false);
        }
        cmd->setWidth(PositiveLength(Length::fromMm(mUi->spbWidth->value())), false); // can throw
        cmd->setHeight(PositiveLength(Length::fromMm(mUi->spbHeight->value())), false); // can throw
        cmd->setDrillDiameter(UnsignedLength(Length::fromMm(mUi->spbDrillDiameter->value())), false); // can throw
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
