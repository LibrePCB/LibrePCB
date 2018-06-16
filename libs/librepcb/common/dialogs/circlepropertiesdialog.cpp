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
#include "circlepropertiesdialog.h"
#include "ui_circlepropertiesdialog.h"
#include "../geometry/circle.h"
#include "../geometry/cmd/cmdcircleedit.h"
#include "../graphics/graphicslayer.h"
#include "../undostack.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

EllipsePropertiesDialog::EllipsePropertiesDialog(Ellipse& ellipse, UndoStack& undoStack,
        QList<GraphicsLayer*> layers, QWidget* parent) noexcept :
    QDialog(parent), mEllipse(ellipse), mUndoStack(undoStack),
    mUi(new Ui::EllipsePropertiesDialog)
{
    mUi->setupUi(this);

    foreach (const GraphicsLayer* layer, layers) {
        mUi->cbxLayer->addItem(layer->getNameTr(), layer->getName());
    }

    connect(mUi->cbxEnableHeightField, &QCheckBox::toggled,
            mUi->spbHeight, &QDoubleSpinBox::setEnabled);
    connect(mUi->buttonBox, &QDialogButtonBox::clicked,
            this, &EllipsePropertiesDialog::buttonBoxClicked);

    // load ellipse attributes
    selectLayerNameInCombobox(mEllipse.getLayerName());
    mUi->spbLineWidth->setValue(mEllipse.getLineWidth().toMm());
    mUi->cbxFillArea->setChecked(mEllipse.isFilled());
    mUi->cbxIsGrabArea->setChecked(mEllipse.isGrabArea());
    mUi->spbWidth->setValue(mEllipse.getRadiusX().toMm() * 2);
    mUi->spbHeight->setValue(mEllipse.getRadiusY().toMm() * 2);
    mUi->cbxEnableHeightField->setChecked(!mEllipse.isRound());
    mUi->spbPosX->setValue(mEllipse.getCenter().getX().toMm());
    mUi->spbPosY->setValue(mEllipse.getCenter().getY().toMm());
    mUi->spbRotation->setValue(mEllipse.getRotation().toDeg());
}

EllipsePropertiesDialog::~EllipsePropertiesDialog() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void EllipsePropertiesDialog::buttonBoxClicked(QAbstractButton* button) noexcept
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

bool EllipsePropertiesDialog::applyChanges() noexcept
{
    try {
        Length radiusX = Length::fromMm(mUi->spbWidth->value() / 2);
        Length radiusY = mUi->spbHeight->isEnabled() ?
                         Length::fromMm(mUi->spbHeight->value() / 2) : radiusX;

        QScopedPointer<CmdEllipseEdit> cmd(new CmdEllipseEdit(mEllipse));
        if (mUi->cbxLayer->currentIndex() >= 0 && mUi->cbxLayer->currentData().isValid()) {
            cmd->setLayerName(mUi->cbxLayer->currentData().toString(), false);
        }
        cmd->setIsFilled(mUi->cbxFillArea->isChecked(), false);
        cmd->setIsGrabArea(mUi->cbxIsGrabArea->isChecked(), false);
        cmd->setLineWidth(Length::fromMm(mUi->spbLineWidth->value()), false);
        cmd->setRadiusX(radiusX, false);
        cmd->setRadiusY(radiusY, false);
        cmd->setCenter(Point::fromMm(mUi->spbPosX->value(), mUi->spbPosY->value()), false);
        cmd->setRotation(Angle::fromDeg(mUi->spbRotation->value()), false);
        mUndoStack.execCmd(cmd.take());
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
        return false;
    }
}

void EllipsePropertiesDialog::selectLayerNameInCombobox(const QString& name) noexcept
{
    mUi->cbxLayer->setCurrentIndex(mUi->cbxLayer->findData(name));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
