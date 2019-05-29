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
#include "compsymbvarpinsignalmapeditorwidget.h"

#include "cmpsigpindisplaytypecombobox.h"

#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

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

CompSymbVarPinSignalMapEditorWidget::CompSymbVarPinSignalMapEditorWidget(
    QWidget* parent) noexcept
  : QWidget(parent),
    mTable(new QTableWidget(this)),
    mWorkspace(nullptr),
    mSignalList(nullptr),
    mSymbolVariant(nullptr) {
  mTable->setCornerButtonEnabled(false);
  mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTable->setSelectionMode(QAbstractItemView::SingleSelection);
  mTable->setWordWrap(false);  // avoid too high cells due to word wrap
  mTable->setColumnCount(_COLUMN_COUNT);
  mTable->setHorizontalHeaderItem(COLUMN_SYMBOL,
                                  new QTableWidgetItem(tr("Symbol")));
  mTable->setHorizontalHeaderItem(COLUMN_PIN,
                                  new QTableWidgetItem(tr("Symbol Pin")));
  mTable->setHorizontalHeaderItem(COLUMN_SIGNAL,
                                  new QTableWidgetItem(tr("Component Signal")));
  mTable->setHorizontalHeaderItem(
      COLUMN_DISPLAYTYPE, new QTableWidgetItem(tr("Designator in Schematics")));
  mTable->horizontalHeader()->setSectionResizeMode(COLUMN_SYMBOL,
                                                   QHeaderView::Stretch);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_PIN, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_SIGNAL, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_DISPLAYTYPE, QHeaderView::ResizeToContents);
  mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  mTable->verticalHeader()->setMinimumSectionSize(20);
  connect(mTable, &QTableWidget::currentCellChanged, this,
          &CompSymbVarPinSignalMapEditorWidget::currentCellChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(3);
  layout->addWidget(mTable);
  QPushButton* btn =
      new QPushButton(tr("Automatically assign all signals by name"), this);
  connect(btn, &QPushButton::clicked, this,
          &CompSymbVarPinSignalMapEditorWidget::btnAutoAssignSignalsClicked);
  layout->addWidget(btn);
}

CompSymbVarPinSignalMapEditorWidget::
    ~CompSymbVarPinSignalMapEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CompSymbVarPinSignalMapEditorWidget::setVariant(
    const workspace::Workspace& ws, const ComponentSignalList& sigs,
    ComponentSymbolVariant& variant) noexcept {
  mWorkspace     = &ws;
  mSignalList    = &sigs;
  mSymbolVariant = &variant;
  mSelectedItem  = tl::nullopt;
  mSelectedPin   = tl::nullopt;
  updateTable();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void CompSymbVarPinSignalMapEditorWidget::currentCellChanged(
    int currentRow, int currentColumn, int previousRow,
    int previousColumn) noexcept {
  Q_UNUSED(currentColumn);
  Q_UNUSED(previousRow);
  Q_UNUSED(previousColumn);
  mSelectedItem = getItemUuidOfRow(currentRow);
  mSelectedPin  = getPinUuidOfRow(currentRow);
}

void CompSymbVarPinSignalMapEditorWidget::componentSignalChanged(
    int index) noexcept {
  QComboBox* cbx = dynamic_cast<QComboBox*>(sender());
  Q_ASSERT(cbx);
  int                row  = getRowOfTableCellWidget(sender());
  tl::optional<Uuid> item = getItemUuidOfRow(row);
  tl::optional<Uuid> pin  = getPinUuidOfRow(row);
  tl::optional<Uuid> signal =
      Uuid::tryFromString(cbx->itemData(index, Qt::UserRole).toString());
  if (item && pin) {
    setComponentSignal(*item, *pin, signal);
  }
}

void CompSymbVarPinSignalMapEditorWidget::displayTypeChanged(
    const CmpSigPinDisplayType& dt) noexcept {
  int                row  = getRowOfTableCellWidget(sender());
  tl::optional<Uuid> item = getItemUuidOfRow(row);
  tl::optional<Uuid> pin  = getPinUuidOfRow(row);
  if (item && pin) {
    setDisplayType(*item, *pin, dt);
  }
}

void CompSymbVarPinSignalMapEditorWidget::
    btnAutoAssignSignalsClicked() noexcept {
  try {
    for (ComponentSymbolVariantItem& item : mSymbolVariant->getSymbolItems()) {
      FilePath fp = mWorkspace->getLibraryDb().getLatestSymbol(
          item.getSymbolUuid());  // can throw
      Symbol symbol(
          std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
              TransactionalFileSystem::openRO(fp))));  // can throw
      for (ComponentPinSignalMapItem& map : item.getPinSignalMap()) {
        CircuitIdentifier pinName =
            symbol.getPins().get(map.getPinUuid())->getName();
        std::shared_ptr<const ComponentSignal> signal =
            mSignalList->find(*pinName);
        map.setSignalUuid(signal ? tl::make_optional(signal->getUuid())
                                 : tl::nullopt);
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
  updateVariant();
  emit edited();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CompSymbVarPinSignalMapEditorWidget::updateTable(
    tl::optional<Uuid> selItem, tl::optional<Uuid> selPin) noexcept {
  blockSignals(true);

  // remove all rows
  int scrollbarValue = mTable->verticalScrollBar()->value();
  int selectedRow    = -1;
  mTable->clearSelection();
  mTable->clearContents();
  mTable->setRowCount(getTotalPinCount());

  // list all pins
  int row = 0;
  for (int i = 0; i < mSymbolVariant->getSymbolItems().count(); ++i) {
    const ComponentSymbolVariantItem& item =
        *mSymbolVariant->getSymbolItems().at(i);
    QScopedPointer<Symbol> symbol;
    try {
      FilePath fp = mWorkspace->getLibraryDb().getLatestSymbol(
          item.getSymbolUuid());  // can throw
      symbol.reset(new Symbol(
          std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
              TransactionalFileSystem::openRO(fp)))));  // can throw
    } catch (const Exception& e) {
      // what could we do here?
    }
    for (const ComponentPinSignalMapItem& mapItem : item.getPinSignalMap()) {
      setTableRowContent(row, item, mapItem, i + 1, symbol.data());
      if (item.getUuid() == selItem && mapItem.getPinUuid() == selPin) {
        selectedRow = row;
      }
      ++row;
    }
  }

  // workaround to trigger column resizing because sometimes auto-resizing does
  // not work
  mTable->hide();
  mTable->show();

  // set selected row
  mTable->verticalScrollBar()->setValue(scrollbarValue);
  mTable->selectRow(selectedRow);
  mSelectedItem = selItem;
  mSelectedPin  = selPin;

  blockSignals(false);
}

void CompSymbVarPinSignalMapEditorWidget::setTableRowContent(
    int row, const ComponentSymbolVariantItem& item,
    const ComponentPinSignalMapItem& map, int itemNumber,
    const Symbol* symbol) noexcept {
  // symbol
  QString symbolName =
      symbol ? *symbol->getNames().value(getLocaleOrder()) : "ERROR";
  QTableWidgetItem* symbolItem = new QTableWidgetItem(symbolName);
  symbolItem->setFlags(symbolItem->flags() &
                       ~Qt::ItemFlags(Qt::ItemIsEditable));
  symbolItem->setData(Qt::UserRole, item.getUuid().toStr());
  mTable->setItem(row, COLUMN_SYMBOL, symbolItem);

  // pin
  const SymbolPin* pin =
      symbol ? symbol->getPins().find(map.getPinUuid()).get() : nullptr;
  QString pinName = QString::number(itemNumber) % "::";
  if (!item.getSuffix()->isEmpty()) pinName.append(item.getSuffix() % "::");
  pinName.append(pin ? *pin->getName() : "ERROR");
  QTableWidgetItem* pinItem = new QTableWidgetItem(pinName);
  pinItem->setFlags(pinItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
  pinItem->setData(Qt::UserRole, map.getPinUuid().toStr());
  mTable->setItem(row, COLUMN_PIN, pinItem);

  // signal
  int        cbxHeight = 23;  // TODO: can we determine this value dynamically?
  QComboBox* signalComboBox = new QComboBox(this);
  signalComboBox->setProperty("row", row);
  signalComboBox->setStyleSheet(
      "padding: 0px 3px 0px 3px;");  // reduce required space
  signalComboBox->setFixedHeight(cbxHeight);
  signalComboBox->setEnabled(symbol && pin ? true : false);
  signalComboBox->addItem(tr("(not connected)"));
  for (const ComponentSignal& sig : *mSignalList) {
    signalComboBox->addItem(*sig.getName(), sig.getUuid().toStr());
  }
  tl::optional<Uuid> currentSignal = map.getSignalUuid();
  int                currentIndex  = currentSignal ? signalComboBox->findData(
                                         currentSignal->toStr(), Qt::UserRole)
                                   : -1;
  signalComboBox->setCurrentIndex(currentIndex > 0 ? currentIndex : 0);
  connect(
      signalComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &CompSymbVarPinSignalMapEditorWidget::componentSignalChanged);
  mTable->setCellWidget(row, COLUMN_SIGNAL, signalComboBox);

  // display type
  CmpSigPinDisplayTypeComboBox* displayTypeComboBox =
      new CmpSigPinDisplayTypeComboBox(this);
  displayTypeComboBox->setProperty("row", row);
  displayTypeComboBox->setStyleSheet(
      "padding: 0px 3px 0px 3px;");  // reduce required space
  displayTypeComboBox->setFixedHeight(cbxHeight);
  displayTypeComboBox->setEnabled(symbol && pin ? true : false);
  displayTypeComboBox->setCurrentItem(map.getDisplayType());
  connect(displayTypeComboBox,
          &CmpSigPinDisplayTypeComboBox::currentItemChanged, this,
          &CompSymbVarPinSignalMapEditorWidget::displayTypeChanged);
  mTable->setCellWidget(row, COLUMN_DISPLAYTYPE, displayTypeComboBox);

  // adjust the height of the row according to the size of the contained widgets
  mTable->verticalHeader()->resizeSection(row, cbxHeight);
}

void CompSymbVarPinSignalMapEditorWidget::setComponentSignal(
    const Uuid& item, const Uuid& pin,
    const tl::optional<Uuid>& signal) noexcept {
  ComponentSymbolVariantItem& i = *mSymbolVariant->getSymbolItems().get(item);
  ComponentPinSignalMapItem&  m = *i.getPinSignalMap().get(pin);
  m.setSignalUuid(signal);
  updateVariant();
  emit edited();
}

void CompSymbVarPinSignalMapEditorWidget::setDisplayType(
    const Uuid& item, const Uuid& pin,
    const CmpSigPinDisplayType& dt) noexcept {
  ComponentSymbolVariantItem& i = *mSymbolVariant->getSymbolItems().get(item);
  ComponentPinSignalMapItem&  m = *i.getPinSignalMap().get(pin);
  m.setDisplayType(dt);
  updateVariant();
  emit edited();
}

int CompSymbVarPinSignalMapEditorWidget::getRowOfTableCellWidget(
    QObject* obj) const noexcept {
  bool ok  = false;
  int  row = obj->property("row").toInt(&ok);
  Q_ASSERT(ok);
  Q_ASSERT(row >= 0 && row < mTable->rowCount());
  return row;
}

tl::optional<Uuid> CompSymbVarPinSignalMapEditorWidget::getItemUuidOfRow(
    int row) const noexcept {
  QTableWidgetItem* item = mTable->item(row, COLUMN_SYMBOL);
  return item ? Uuid::tryFromString(item->data(Qt::UserRole).toString())
              : tl::nullopt;
}

tl::optional<Uuid> CompSymbVarPinSignalMapEditorWidget::getPinUuidOfRow(
    int row) const noexcept {
  QTableWidgetItem* item = mTable->item(row, COLUMN_PIN);
  return item ? Uuid::tryFromString(item->data(Qt::UserRole).toString())
              : tl::nullopt;
}

int CompSymbVarPinSignalMapEditorWidget::getTotalPinCount() const noexcept {
  int n = 0;
  for (int i = 0; i < mSymbolVariant->getSymbolItems().count(); ++i) {
    n += mSymbolVariant->getSymbolItems().value(i)->getPinSignalMap().count();
  }
  return n;
}

const QStringList& CompSymbVarPinSignalMapEditorWidget::getLocaleOrder() const
    noexcept {
  return mWorkspace->getSettings().getLibLocaleOrder().getLocaleOrder();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
