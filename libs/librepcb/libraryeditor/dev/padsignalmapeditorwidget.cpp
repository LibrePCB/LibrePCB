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
#include "padsignalmapeditorwidget.h"
#include <librepcb/common/toolbox.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/dev/cmd/cmddevicepadsignalmapitemedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PadSignalMapEditorWidget::PadSignalMapEditorWidget(QWidget* parent) noexcept :
    QWidget(parent), mTable(new QTableWidget(this)),
    mUndoStack(nullptr), mPadSignalMap(nullptr)
{
    mTable->setCornerButtonEnabled(false);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setColumnCount(_COLUMN_COUNT);
    mTable->setHorizontalHeaderItem(COLUMN_PAD,     new QTableWidgetItem(tr("Pad Name")));
    mTable->setHorizontalHeaderItem(COLUMN_SIGNAL,  new QTableWidgetItem(tr("Component Signal")));
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_PAD,     QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_SIGNAL,  QHeaderView::Stretch);
    mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mTable->verticalHeader()->setMinimumSectionSize(20);
    connect(mTable, &QTableWidget::currentCellChanged,
            this, &PadSignalMapEditorWidget::currentCellChanged);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mTable);

    updateTable();
}

PadSignalMapEditorWidget::~PadSignalMapEditorWidget() noexcept
{
   setReferences(nullptr, nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void PadSignalMapEditorWidget::setReferences(UndoStack* undoStack, DevicePadSignalMap* map) noexcept
{
    if (mPadSignalMap) {
        mPadSignalMap->unregisterObserver(this);
        for (const DevicePadSignalMapItem& item : *mPadSignalMap) {
            disconnect(&item, &DevicePadSignalMapItem::signalUuidChanged,
                       this, &PadSignalMapEditorWidget::updateTable);
        }
    }
    mUndoStack = undoStack;
    mPadSignalMap = map;
    if (mPadSignalMap) {
        mPadSignalMap->registerObserver(this);
        for (const DevicePadSignalMapItem& item : *mPadSignalMap) {
            connect(&item, &DevicePadSignalMapItem::signalUuidChanged,
                    this, &PadSignalMapEditorWidget::updateTable);
        }
    }
    updateTable();
}

void PadSignalMapEditorWidget::setPadList(const PackagePadList& list) noexcept
{
    mPads = list;
    updateTable();
}

void PadSignalMapEditorWidget::setSignalList(const ComponentSignalList& list) noexcept
{
    mSignals = list;
    updateTable();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void PadSignalMapEditorWidget::currentCellChanged(int currentRow, int currentColumn,
                                                  int previousRow, int previousColumn) noexcept
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    mSelectedPad = getPadUuidOfRow(currentRow);
}

void PadSignalMapEditorWidget::componentSignalChanged(int index) noexcept
{
    QComboBox* cbx = dynamic_cast<QComboBox*>(sender()); Q_ASSERT(cbx);
    tl::optional<Uuid> pad = getPadUuidOfTableCellWidget(sender());
    if (pad) {
        tl::optional<Uuid> signal = Uuid::tryFromString(cbx->itemData(index, Qt::UserRole).toString());
        if (mUndoStack && mPadSignalMap) setComponentSignal(*pad, signal);
    }
}

/*****************************************************************************************
 *  Observer
 ****************************************************************************************/

void PadSignalMapEditorWidget::listObjectAdded(const DevicePadSignalMap& list, int newIndex,
    const std::shared_ptr<DevicePadSignalMapItem>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == mPadSignalMap);
    connect(ptr.get(), &DevicePadSignalMapItem::signalUuidChanged,
            this, &PadSignalMapEditorWidget::updateTable);
    updateTable();
}

void PadSignalMapEditorWidget::listObjectRemoved(const DevicePadSignalMap& list, int oldIndex,
    const std::shared_ptr<DevicePadSignalMapItem>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == mPadSignalMap);
    disconnect(ptr.get(), &DevicePadSignalMapItem::signalUuidChanged,
               this, &PadSignalMapEditorWidget::updateTable);
    updateTable();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PadSignalMapEditorWidget::updateTable() noexcept
{
    blockSignals(true);

    // remove all rows
    int scrollbarValue = mTable->verticalScrollBar()->value();
    int selectedRow = -1;
    mTable->clearSelection();
    mTable->clearContents();

    if (mUndoStack && mPadSignalMap) {
        mTable->setEnabled(true);

        // list all pads
        mTable->setRowCount(mPadSignalMap->count());
        for (int row = 0; row < mPads.count(); ++row) {
            const PackagePad& pad = *mPads.at(row);
            setTableRowContent(row, pad.getUuid(), *pad.getName(),
                DevicePadSignalMapHelpers::tryGetSignalUuid(*mPadSignalMap, pad.getUuid()));
            if (pad.getUuid() == mSelectedPad) {
                selectedRow = row;
            }
        }

        // workaround to trigger column resizing because sometimes auto-resizing does not work
        mTable->hide(); mTable->show();

        // order by pad name
        mTable->sortByColumn(COLUMN_PAD, Qt::AscendingOrder);

        // set selected row
        mTable->verticalScrollBar()->setValue(scrollbarValue);
        mTable->selectRow(selectedRow);
        mSelectedPad = getPadUuidOfRow(selectedRow);
    } else {
        mTable->setEnabled(false);
    }

    blockSignals(false);
}

void PadSignalMapEditorWidget::setTableRowContent(int row, const Uuid& padUuid,
    const QString& padName, const tl::optional<Uuid>& signalUuid) noexcept
{
    // header
    QTableWidgetItem* headerItem = new QTableWidgetItem(padUuid.toStr());
    QFont headerFont = headerItem->font();
    headerFont.setStyleHint(QFont::Monospace); // ensure that the column width is fixed
    headerFont.setFamily("Monospace");
    headerItem->setFont(headerFont);
    mTable->setVerticalHeaderItem(row, headerItem);

    // pad
    QTableWidgetItem* padItem = new QTableWidgetItem();
    padItem->setFlags(padItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
    padItem->setData(Qt::DisplayRole, Toolbox::stringOrNumberToQVariant(padName));
    padItem->setData(Qt::UserRole, padUuid.toStr());
    mTable->setItem(row, COLUMN_PAD, padItem);

    // signal
    int cbxHeight = 23; // TODO: can we determine this value dynamically?
    QComboBox* signalComboBox = new QComboBox(this);
    signalComboBox->setProperty("pad", padUuid.toStr());
    signalComboBox->setStyleSheet("padding: 0px 3px 0px 3px;"); // reduce required space
    signalComboBox->setFixedHeight(cbxHeight);
    signalComboBox->addItem(tr("(not connected)"));
    for (const ComponentSignal& sig : mSignals) {
        signalComboBox->addItem(*sig.getName(), sig.getUuid().toStr());
    }
    int currentIndex = signalUuid ? signalComboBox->findData(signalUuid->toStr(), Qt::UserRole) : -1;
    signalComboBox->setCurrentIndex(currentIndex > 0 ? currentIndex : 0);
    connect(signalComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PadSignalMapEditorWidget::componentSignalChanged);
    mTable->setCellWidget(row, COLUMN_SIGNAL, signalComboBox);

    // adjust the height of the row according to the size of the contained widgets
    mTable->verticalHeader()->resizeSection(row, cbxHeight);
}

void PadSignalMapEditorWidget::setComponentSignal(const Uuid& pad, const tl::optional<Uuid>& signal) noexcept
{
    try {
        DevicePadSignalMapItem& item = *mPadSignalMap->get(pad); // can throw
        QScopedPointer<CmdDevicePadSignalMapItemEdit> cmd(
            new CmdDevicePadSignalMapItemEdit(item));
        cmd->setSignalUuid(signal);
        mUndoStack->execCmd(cmd.take());
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not set signal"), e.getMsg());
    }
}

tl::optional<Uuid> PadSignalMapEditorWidget::getPadUuidOfTableCellWidget(QObject* obj) const noexcept
{
    return Uuid::tryFromString(obj->property("pad").toString());
}

tl::optional<Uuid> PadSignalMapEditorWidget::getPadUuidOfRow(int row) const noexcept
{
    QTableWidgetItem* item = mTable->item(row, COLUMN_PAD);
    return item ? Uuid::tryFromString(item->data(Qt::UserRole).toString()) : tl::nullopt;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
