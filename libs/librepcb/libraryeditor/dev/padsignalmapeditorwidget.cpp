/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "padsignalmapeditorwidget.h"

#include <librepcb/common/toolbox.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/dev/cmd/cmddevicepadsignalmapitemedit.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PadSignalMapEditorWidget::PadSignalMapEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mTable(new QTableWidget(this)),
    mUndoStack(nullptr),
    mPadSignalMap(nullptr),
    mMapEditedSlot(*this, &PadSignalMapEditorWidget::mapEdited) {
  mTable->setCornerButtonEnabled(false);
  mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTable->setSelectionMode(QAbstractItemView::SingleSelection);
  mTable->setWordWrap(false);  // avoid too high cells due to word wrap
  mTable->setColumnCount(_COLUMN_COUNT);
  mTable->setHorizontalHeaderItem(COLUMN_PAD,
                                  new QTableWidgetItem(tr("Pad Name")));
  mTable->setHorizontalHeaderItem(COLUMN_SIGNAL,
                                  new QTableWidgetItem(tr("Component Signal")));
  mTable->horizontalHeader()->setSectionResizeMode(COLUMN_PAD,
                                                   QHeaderView::Stretch);
  mTable->horizontalHeader()->setSectionResizeMode(COLUMN_SIGNAL,
                                                   QHeaderView::Stretch);
  mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  mTable->verticalHeader()->setMinimumSectionSize(20);
  connect(mTable, &QTableWidget::currentCellChanged, this,
          &PadSignalMapEditorWidget::currentCellChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mTable);

  updateTable();
}

PadSignalMapEditorWidget::~PadSignalMapEditorWidget() noexcept {
  setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PadSignalMapEditorWidget::setReferences(UndoStack*          undoStack,
                                             DevicePadSignalMap* map) noexcept {
  if (mPadSignalMap) {
    mPadSignalMap->onEdited.detach(mMapEditedSlot);
  }
  mUndoStack    = undoStack;
  mPadSignalMap = map;
  if (mPadSignalMap) {
    mPadSignalMap->onEdited.attach(mMapEditedSlot);
  }
  updateTable();
}

void PadSignalMapEditorWidget::setPadList(const PackagePadList& list) noexcept {
  mPads = list;
  updateTable();
}

void PadSignalMapEditorWidget::setSignalList(
    const ComponentSignalList& list) noexcept {
  mSignals = list;
  updateTable();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void PadSignalMapEditorWidget::currentCellChanged(int currentRow,
                                                  int currentColumn,
                                                  int previousRow,
                                                  int previousColumn) noexcept {
  Q_UNUSED(currentColumn);
  Q_UNUSED(previousRow);
  Q_UNUSED(previousColumn);
  mSelectedPad = getPadUuidOfRow(currentRow);
}

void PadSignalMapEditorWidget::componentSignalChanged(int index) noexcept {
  QComboBox* cbx = dynamic_cast<QComboBox*>(sender());
  Q_ASSERT(cbx);
  tl::optional<Uuid> pad = getPadUuidOfTableCellWidget(sender());
  if (pad) {
    tl::optional<Uuid> signal =
        Uuid::tryFromString(cbx->itemData(index, Qt::UserRole).toString());
    if (mUndoStack && mPadSignalMap) setComponentSignal(*pad, signal);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PadSignalMapEditorWidget::mapEdited(
    const DevicePadSignalMap& map, int index,
    const std::shared_ptr<const DevicePadSignalMapItem>& item,
    DevicePadSignalMap::Event                            event) noexcept {
  Q_UNUSED(map);
  Q_UNUSED(index);
  Q_UNUSED(item);
  Q_UNUSED(event);
  updateTable();
}

void PadSignalMapEditorWidget::updateTable() noexcept {
  blockSignals(true);

  // remove all rows
  int scrollbarValue = mTable->verticalScrollBar()->value();
  int selectedRow    = -1;
  mTable->clearSelection();
  mTable->clearContents();

  if (mUndoStack && mPadSignalMap) {
    mTable->setEnabled(true);

    // list all pads
    mTable->setRowCount(mPadSignalMap->count());
    for (int row = 0; row < mPads.count(); ++row) {
      const PackagePad& pad = *mPads.at(row);
      setTableRowContent(row, pad.getUuid(), *pad.getName(),
                         DevicePadSignalMapHelpers::tryGetSignalUuid(
                             *mPadSignalMap, pad.getUuid()));
      if (pad.getUuid() == mSelectedPad) {
        selectedRow = row;
      }
    }

    // workaround to trigger column resizing because sometimes auto-resizing
    // does not work
    mTable->hide();
    mTable->show();

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

void PadSignalMapEditorWidget::setTableRowContent(
    int row, const Uuid& padUuid, const QString& padName,
    const tl::optional<Uuid>& signalUuid) noexcept {
  // header
  QTableWidgetItem* headerItem = new QTableWidgetItem(padUuid.toStr());
  QFont             headerFont = headerItem->font();
  headerFont.setStyleHint(
      QFont::Monospace);  // ensure that the column width is fixed
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
  int        cbxHeight = 23;  // TODO: can we determine this value dynamically?
  QComboBox* signalComboBox = new QComboBox(this);
  signalComboBox->setProperty("pad", padUuid.toStr());
  signalComboBox->setStyleSheet(
      "padding: 0px 3px 0px 3px;");  // reduce required space
  signalComboBox->setFixedHeight(cbxHeight);
  for (const ComponentSignal& sig : mSignals) {
    signalComboBox->addItem(*sig.getName(), sig.getUuid().toStr());
    // Set display role to a QVariant to get numbers sorted by value and strings
    // alphabetically.
    signalComboBox->setItemData(
        signalComboBox->count() - 1,
        Toolbox::stringOrNumberToQVariant(*sig.getName()), Qt::DisplayRole);
  }
  signalComboBox->model()->sort(0);
  signalComboBox->insertItem(0, tr("(not connected)"));
  int currentIndex =
      signalUuid ? signalComboBox->findData(signalUuid->toStr(), Qt::UserRole)
                 : -1;
  signalComboBox->setCurrentIndex(currentIndex > 0 ? currentIndex : 0);
  connect(
      signalComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &PadSignalMapEditorWidget::componentSignalChanged);
  mTable->setCellWidget(row, COLUMN_SIGNAL, signalComboBox);

  // adjust the height of the row according to the size of the contained widgets
  mTable->verticalHeader()->resizeSection(row, cbxHeight);
}

void PadSignalMapEditorWidget::setComponentSignal(
    const Uuid& pad, const tl::optional<Uuid>& signal) noexcept {
  try {
    DevicePadSignalMapItem& item = *mPadSignalMap->get(pad);  // can throw
    QScopedPointer<CmdDevicePadSignalMapItemEdit> cmd(
        new CmdDevicePadSignalMapItemEdit(item));
    cmd->setSignalUuid(signal);
    mUndoStack->execCmd(cmd.take());
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not set signal"), e.getMsg());
  }
}

tl::optional<Uuid> PadSignalMapEditorWidget::getPadUuidOfTableCellWidget(
    QObject* obj) const noexcept {
  return Uuid::tryFromString(obj->property("pad").toString());
}

tl::optional<Uuid> PadSignalMapEditorWidget::getPadUuidOfRow(int row) const
    noexcept {
  QTableWidgetItem* item = mTable->item(row, COLUMN_PAD);
  return item ? Uuid::tryFromString(item->data(Qt::UserRole).toString())
              : tl::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
