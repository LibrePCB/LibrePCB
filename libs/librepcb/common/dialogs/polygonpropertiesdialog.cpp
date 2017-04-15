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
#include "polygonpropertiesdialog.h"
#include "ui_polygonpropertiesdialog.h"
#include "../geometry/polygon.h"
#include "../geometry/cmd/cmdpolygonedit.h"
#include "../geometry/cmd/cmdpolygonsegmentedit.h"
#include "../graphics/graphicslayer.h"
#include "../undostack.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

PolygonPropertiesDialog::PolygonPropertiesDialog(Polygon& polygon,
        UndoStack& undoStack, QList<GraphicsLayer*> layers, QWidget* parent) noexcept :
    QDialog(parent), mPolygon(polygon), mUndoStack(undoStack),
    mUi(new Ui::PolygonPropertiesDialog)
{
    mUi->setupUi(this);

    foreach (const GraphicsLayer* layer, layers) {
        mUi->cbxLayer->addItem(layer->getNameTr(), layer->getName());
    }

    connect(mUi->buttonBox, &QDialogButtonBox::clicked,
            this, &PolygonPropertiesDialog::buttonBoxClicked);

    // load polygon attributes
    selectLayerNameInCombobox(mPolygon.getLayerName());
    mUi->spbLineWidth->setValue(mPolygon.getLineWidth().toMm());
    mUi->cbxFillArea->setChecked(mPolygon.isFilled());
    mUi->cbxIsGrabArea->setChecked(mPolygon.isGrabArea());

    // load segments
    mUi->tableWidget->setRowCount(mPolygon.getSegments().count() + 1);
    setSegmentsTableRow(0, mPolygon.getStartPos(), Angle::deg0());
    for (int i = 0; i < mPolygon.getSegments().count(); ++i) {
        const PolygonSegment& segment = *mPolygon.getSegments().at(i);
        setSegmentsTableRow(i + 1, segment.getEndPos(), segment.getAngle());
    }
}

PolygonPropertiesDialog::~PolygonPropertiesDialog() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PolygonPropertiesDialog::buttonBoxClicked(QAbstractButton* button) noexcept
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

bool PolygonPropertiesDialog::applyChanges() noexcept
{
    try {
        UndoStackTransaction transaction(mUndoStack, tr("Edit polygon"));
        // polygon
        QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(mPolygon));
        if (mUi->cbxLayer->currentIndex() >= 0 && mUi->cbxLayer->currentData().isValid()) {
            cmd->setLayerName(mUi->cbxLayer->currentData().toString(), false);
        }
        cmd->setIsFilled(mUi->cbxFillArea->isChecked(), false);
        cmd->setIsGrabArea(mUi->cbxIsGrabArea->isChecked(), false);
        cmd->setLineWidth(Length::fromMm(mUi->spbLineWidth->value()), false);
        cmd->setStartPos(getSegmentsTableRowPos(0), false);
        transaction.append(cmd.take());
        // segments
        for (int i = 1; i < mUi->tableWidget->rowCount(); ++i) {
            PolygonSegment& segment = *mPolygon.getSegments()[i - 1];
            QScopedPointer<CmdPolygonSegmentEdit> cmd(new CmdPolygonSegmentEdit(segment));
            cmd->setEndPos(getSegmentsTableRowPos(i), false);
            cmd->setAngle(getSegmentsTableRowAngle(i), false);
            transaction.append(cmd.take());
        }
        transaction.commit();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
        return false;
    }
}

void PolygonPropertiesDialog::setSegmentsTableRow(int row, const Point& pos,
                                                  const Angle& angle) noexcept
{
    mUi->tableWidget->setItem(row, 0, new QTableWidgetItem(pos.getX().toMmString()));
    mUi->tableWidget->setItem(row, 1, new QTableWidgetItem(pos.getY().toMmString()));
    QTableWidgetItem* angleItem = new QTableWidgetItem(angle.toDegString());
    if (row == 0) angleItem->setFlags(Qt::NoItemFlags);
    mUi->tableWidget->setItem(row, 2, angleItem);
}

Point PolygonPropertiesDialog::getSegmentsTableRowPos(int row)
{
    QTableWidgetItem* col0 = mUi->tableWidget->item(row, 0); Q_ASSERT(col0);
    QTableWidgetItem* col1 = mUi->tableWidget->item(row, 1); Q_ASSERT(col1);
    return Point(Length::fromMm(col0->text()), Length::fromMm(col1->text()));
}

Angle PolygonPropertiesDialog::getSegmentsTableRowAngle(int row)
{
    QTableWidgetItem* col2 = mUi->tableWidget->item(row, 2); Q_ASSERT(col2);
    return Angle::fromDeg(col2->text());
}

void PolygonPropertiesDialog::selectLayerNameInCombobox(const QString& name) noexcept
{
    mUi->cbxLayer->setCurrentIndex(mUi->cbxLayer->findData(name));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
