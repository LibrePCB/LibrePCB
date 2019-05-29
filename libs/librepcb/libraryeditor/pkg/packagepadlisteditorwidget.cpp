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
#include "packagepadlisteditorwidget.h"

#include <librepcb/common/undostack.h>
#include <librepcb/library/pkg/cmd/cmdpackagepadedit.h>

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

PackagePadListEditorWidget::PackagePadListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mTable(new QTableWidget(this)),
    mPadList(nullptr),
    mUndoStack(nullptr),
    mPadListEditedSlot(*this, &PackagePadListEditorWidget::padListEdited) {
  mTable->setCornerButtonEnabled(false);
  mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTable->setSelectionMode(QAbstractItemView::SingleSelection);
  mTable->setWordWrap(false);  // avoid too high cells due to word wrap
  mTable->setColumnCount(_COLUMN_COUNT);
  mTable->setHorizontalHeaderItem(COLUMN_NAME,
                                  new QTableWidgetItem(tr("Name")));
  mTable->setHorizontalHeaderItem(COLUMN_BUTTONS, new QTableWidgetItem(""));
  mTable->horizontalHeader()->setSectionResizeMode(COLUMN_NAME,
                                                   QHeaderView::Stretch);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_BUTTONS, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setMinimumSectionSize(10);
  mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  mTable->verticalHeader()->setMinimumSectionSize(20);
  mTable->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
  connect(mTable, &QTableWidget::currentCellChanged, this,
          &PackagePadListEditorWidget::currentCellChanged);
  connect(mTable, &QTableWidget::cellChanged, this,
          &PackagePadListEditorWidget::tableCellChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mTable);
}

PackagePadListEditorWidget::~PackagePadListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackagePadListEditorWidget::setReferences(PackagePadList& list,
                                               UndoStack*      stack) noexcept {
  if (mPadList) {
    mPadList->onEdited.detach(mPadListEditedSlot);
  }
  mPadList     = &list;
  mUndoStack   = stack;
  mSelectedPad = tl::nullopt;
  if (mPadList) {
    mPadList->onEdited.attach(mPadListEditedSlot);
  }
  updateTable();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void PackagePadListEditorWidget::currentCellChanged(
    int currentRow, int currentColumn, int previousRow,
    int previousColumn) noexcept {
  Q_UNUSED(currentColumn);
  Q_UNUSED(previousRow);
  Q_UNUSED(previousColumn);
  mSelectedPad = getUuidOfRow(currentRow);
}

void PackagePadListEditorWidget::tableCellChanged(int row,
                                                  int column) noexcept {
  QTableWidgetItem* item = mTable->item(row, column);
  Q_ASSERT(item);
  tl::optional<Uuid> uuid = getUuidOfRow(row);

  if (isNewPadRow(row)) {
    if (column == COLUMN_NAME) {
      item->setText(cleanCircuitIdentifier(item->text()));
    }
  } else if (isExistingPadRow(row) && uuid) {
    if (column == COLUMN_NAME) {
      item->setText(*setName(*uuid, cleanCircuitIdentifier(item->text())));
    }
  }
}

void PackagePadListEditorWidget::btnAddRemoveClicked() noexcept {
  int                row  = getRowOfTableCellWidget(sender());
  tl::optional<Uuid> uuid = getUuidOfRow(row);
  if (isNewPadRow(row)) {
    const QTableWidgetItem* nameItem = mTable->item(row, COLUMN_NAME);
    Q_ASSERT(nameItem);
    QString name = cleanCircuitIdentifier(nameItem->text());
    addPad(!name.isEmpty() ? name : getNextPadNameProposal());
  } else if (isExistingPadRow(row) && uuid) {
    removePad(*uuid);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackagePadListEditorWidget::padListEdited(
    const PackagePadList& list, int index,
    const std::shared_ptr<const PackagePad>& pad,
    PackagePadList::Event                    event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(pad);
  Q_UNUSED(event);
  updateTable(mSelectedPad);
}

void PackagePadListEditorWidget::updateTable(
    const tl::optional<Uuid>& selected) noexcept {
  mTable->blockSignals(true);

  // remove all rows
  int selectedRow = newPadRow();
  mTable->clearSelection();
  mTable->clearContents();
  mTable->setRowCount(mPadList->count() + 1);

  // special row for adding a new pad
  setTableRowContent(newPadRow(), tl::nullopt, "");

  // existing signals
  for (int i = 0; i < mPadList->count(); ++i) {
    const PackagePad& pad = *mPadList->at(i);
    setTableRowContent(indexToRow(i), pad.getUuid(), *pad.getName());
    if (pad.getUuid() == selected) {
      selectedRow = indexToRow(i);
    }
  }

  // workaround to trigger column resizing because sometimes auto-resizing does
  // not work
  mTable->hide();
  mTable->show();

  // set selected row
  mTable->selectRow(selectedRow);
  mSelectedPad = selected;

  mTable->blockSignals(false);
}

void PackagePadListEditorWidget::setTableRowContent(
    int row, const tl::optional<Uuid>& uuid, const QString& name) noexcept {
  // header
  QString header = uuid ? uuid->toStr().left(13) % "..." : tr("Add new pad:");
  QTableWidgetItem* headerItem = new QTableWidgetItem(header);
  headerItem->setToolTip(uuid ? uuid->toStr() : QString());
  QFont headerFont = headerItem->font();
  headerFont.setStyleHint(
      QFont::Monospace);  // ensure that the column width is fixed
  headerFont.setFamily("Monospace");
  headerItem->setFont(headerFont);
  mTable->setVerticalHeaderItem(row, headerItem);

  // name
  mTable->setItem(row, COLUMN_NAME, new QTableWidgetItem(name));

  // Adjust the height of the row according to the size of the contained
  // widgets. This needs to be done *before* adding the button, as the button
  // would increase the row height!
  mTable->resizeRowToContents(row);

  // button
  int          btnSize      = mTable->rowHeight(row);
  QToolButton* btnAddRemove = new QToolButton(this);
  btnAddRemove->setProperty("row", row);
  btnAddRemove->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  btnAddRemove->setFixedSize(btnSize, btnSize);
  btnAddRemove->setIconSize(QSize(btnSize - 6, btnSize - 6));
  connect(btnAddRemove, &QToolButton::clicked, this,
          &PackagePadListEditorWidget::btnAddRemoveClicked);
  if (isExistingPadRow(row)) {
    btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));
  } else {
    btnAddRemove->setIcon(QIcon(":/img/actions/add.png"));
  }
  mTable->setCellWidget(row, COLUMN_BUTTONS, btnAddRemove);
}

void PackagePadListEditorWidget::addPad(const QString& name) noexcept {
  try {
    CircuitIdentifier constrainedName = validateNameOrThrow(name);  // can throw
    executeCommand(new CmdPackagePadInsert(
        *mPadList, std::make_shared<PackagePad>(
                       Uuid::createRandom(), constrainedName)));  // can throw
    // updateTable();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not add pad"), e.getMsg());
  }
}

void PackagePadListEditorWidget::removePad(const Uuid& uuid) noexcept {
  try {
    const PackagePad* pad = mPadList->get(uuid).get();        // can throw
    executeCommand(new CmdPackagePadRemove(*mPadList, pad));  // can throw
    // updateTable(mSelectedPad);
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not remove footprint"), e.getMsg());
  }
}

CircuitIdentifier PackagePadListEditorWidget::setName(
    const Uuid& uuid, const QString& name) noexcept {
  PackagePad* pad = mPadList->find(uuid).get();
  Q_ASSERT(pad);
  if (pad->getName() == name) {
    return pad->getName();
  }

  try {
    CircuitIdentifier constrainedName = validateNameOrThrow(name);  // can throw
    QScopedPointer<CmdPackagePadEdit> cmd(new CmdPackagePadEdit(*pad));
    cmd->setName(constrainedName);
    executeCommand(cmd.take());
    return constrainedName;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Invalid name"), e.getMsg());
    return pad->getName();
  }
}

int PackagePadListEditorWidget::getRowOfTableCellWidget(QObject* obj) const
    noexcept {
  bool ok  = false;
  int  row = obj->property("row").toInt(&ok);
  Q_ASSERT(ok);
  Q_ASSERT(row >= 0 && row < mTable->rowCount());
  return row;
}

tl::optional<Uuid> PackagePadListEditorWidget::getUuidOfRow(int row) const
    noexcept {
  if (isExistingPadRow(row)) {
    return mPadList->value(rowToIndex(row))->getUuid();
  } else {
    return tl::nullopt;
  }
}

CircuitIdentifier PackagePadListEditorWidget::validateNameOrThrow(
    const QString& name) const {
  if (mPadList->contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("There is already a pad with the name \"%1\".")).arg(name));
  }
  return CircuitIdentifier(name);  // can throw
}

void PackagePadListEditorWidget::executeCommand(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);  // can throw
  } else {
    QScopedPointer<UndoCommand> guardedCmd(cmd);
    guardedCmd->execute();  // can throw
  }
}

QString PackagePadListEditorWidget::getNextPadNameProposal() const noexcept {
  int i = 1;
  while (mPadList->contains(QString::number(i))) {
    ++i;
  }
  return QString::number(i);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
