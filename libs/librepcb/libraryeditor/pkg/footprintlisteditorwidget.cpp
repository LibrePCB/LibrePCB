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
#include "footprintlisteditorwidget.h"
#include <librepcb/common/undostack.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/library/pkg/cmd/cmdfootprintedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintListEditorWidget::FootprintListEditorWidget(QWidget* parent) noexcept :
    QWidget(parent), mTable(new QTableWidget(this)),
    mFootprintList(nullptr), mUndoStack(nullptr)
{
    mTable->setCornerButtonEnabled(false);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setColumnCount(_COLUMN_COUNT);
    mTable->setHorizontalHeaderItem(COLUMN_UUID,    new QTableWidgetItem(tr("UUID")));
    mTable->setHorizontalHeaderItem(COLUMN_NAME,    new QTableWidgetItem(tr("Name")));
    mTable->setHorizontalHeaderItem(COLUMN_BUTTONS, new QTableWidgetItem(""));
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_UUID,    QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_NAME,    QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_BUTTONS, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setMinimumSectionSize(10);
    mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mTable->verticalHeader()->setMinimumSectionSize(20);
    //mTable->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    mTable->verticalHeader()->setVisible(false);
    mSortAsc = true;
    QHeaderView *header = mTable->horizontalHeader();
    connect(header, &QHeaderView::sectionClicked,
            this, &FootprintListEditorWidget::HeaderClicked);
    connect(mTable, &QTableWidget::currentCellChanged,
            this, &FootprintListEditorWidget::currentCellChanged);
    connect(mTable, &QTableWidget::cellChanged,
            this, &FootprintListEditorWidget::tableCellChanged);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mTable);
}

FootprintListEditorWidget::~FootprintListEditorWidget() noexcept
{
    if (mFootprintList) mFootprintList->unregisterObserver(this);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void FootprintListEditorWidget::setReferences(FootprintList& list, UndoStack& stack) noexcept
{
    mFootprintList = &list;
    mUndoStack = &stack;
    mSelectedFootprint = Uuid();
    mFootprintList->registerObserver(this);
    updateTable();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/
void FootprintListEditorWidget::HeaderClicked(int ind){
    if(ind == 1){
        if(mSortAsc){
            mSortAsc = false;
        }
        else{
            mSortAsc = true;
        }
    }
    updateTable();
}

void FootprintListEditorWidget::currentCellChanged(int currentRow, int currentColumn,
                                                   int previousRow, int previousColumn) noexcept
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    mSelectedFootprint = getUuidOfRow(currentRow);
    emit currentFootprintChanged(rowToIndex(currentRow));
}

void FootprintListEditorWidget::tableCellChanged(int row, int column) noexcept
{
    QTableWidgetItem* item = mTable->item(row, column); Q_ASSERT(item);

    if (isNewFootprintRow(row)) {
        if (column == COLUMN_NAME) {
            item->setText(cleanName(item->text()));
        }
    } else if (isExistingFootprintRow(row)) {
        if (column == COLUMN_NAME) {
            item->setText(setName(getUuidOfRow(row), cleanName(item->text())));
        }
    }
}

void FootprintListEditorWidget::btnUpClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (!isExistingFootprintRow(row)) return;
    int index = rowToIndex(row);
    if (index <= 0) return;
    moveFootprintUp(index);
}

void FootprintListEditorWidget::btnDownClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (!isExistingFootprintRow(row)) return;
    int index = rowToIndex(row);
    if (index >= mFootprintList->count() - 1) return;
    moveFootprintDown(index);
}

void FootprintListEditorWidget::btnCopyClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (isExistingFootprintRow(row)) {
        copyFootprint(getUuidOfRow(row));
    }
}

void FootprintListEditorWidget::btnAddRemoveClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (isNewFootprintRow(row)) {
        const QTableWidgetItem* nameItem = mTable->item(row, COLUMN_NAME); Q_ASSERT(nameItem);
        addFootprint(cleanName(nameItem->text()));
    } else if (isExistingFootprintRow(row)) {
        removeFootprint(getUuidOfRow(row));
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void FootprintListEditorWidget::updateTable(Uuid selected) noexcept
{
    mTable->blockSignals(true);

    // selecte the first row by default to make sure a footprint is shown in the grpahics view
    if (selected.isNull() && !mFootprintList->isEmpty()) {
        selected = mFootprintList->first()->getUuid();
    }

    // remove all rows
    int selectedRow = newFootprintRow();
    mTable->clearSelection();
    mTable->clearContents();
    mTable->setRowCount(mFootprintList->count() + 1);

    // special row for adding a new footprint
    setTableRowContent(newFootprintRow(), Uuid(), "");

    // existing signals
    for (int i = 0; i < mFootprintList->count(); ++i) {
        const Footprint& footprint = *mFootprintList->at(i);
        setTableRowContent(indexToRow(i), footprint.getUuid(), footprint.getNames().getDefaultValue());
        if (footprint.getUuid() == selected) {
            selectedRow = indexToRow(i);
        }
    }

    // workaround to trigger column resizing because sometimes auto-resizing does not work
    mTable->hide(); mTable->show();

    // set selected row
    mTable->selectRow(selectedRow);
    mSelectedFootprint = selected;
    emit currentFootprintChanged(rowToIndex(selectedRow));

    mTable->blockSignals(false);
}

void FootprintListEditorWidget::setTableRowContent(int row, const Uuid& uuid,
                                                    const QString& name) noexcept
{
    // header
    QString header = uuid.isNull() ? tr("Add new footprint:") : uuid.toStr().left(13) % "...";
    QTableWidgetItem* headerItem = new QTableWidgetItem(header);
    headerItem->setToolTip(uuid.toStr());
    QFont headerFont = headerItem->font();
    headerFont.setStyleHint(QFont::Monospace); // ensure that the column width is fixed
    headerFont.setFamily("Monospace");
    headerItem->setFont(headerFont);
//    mTable->setVerticalHeaderItem(row, headerItem);
    headerItem->setFlags(headerItem->flags() ^ Qt::ItemIsEditable);
    QPalette pallet = mTable->verticalHeader()->palette();
    headerItem->setBackgroundColor(pallet.background().color());
    mTable->setItem(row, COLUMN_UUID, headerItem);

    // name
    mTable->setItem(row, COLUMN_NAME, new QTableWidgetItem(name));

    // buttons
    int btnSize = 23; // TODO: can we determine this value dynamically?
    QSize iconSize(btnSize - 6, btnSize - 6);
    QWidget* buttonsColumnWidget = new QWidget(this);
    buttonsColumnWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    QHBoxLayout* buttonsColumnLayout = new QHBoxLayout(buttonsColumnWidget);
    buttonsColumnLayout->setContentsMargins(0, 0, 0, 0);
    buttonsColumnLayout->setSpacing(0);
    QToolButton* btnAddRemove = new QToolButton(buttonsColumnWidget);
    btnAddRemove->setProperty("row", row);
    btnAddRemove->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    btnAddRemove->setFixedHeight(btnSize);
    btnAddRemove->setIconSize(iconSize);
    connect(btnAddRemove, &QToolButton::clicked,
            this, &FootprintListEditorWidget::btnAddRemoveClicked);
    if (isExistingFootprintRow(row)) {
        btnAddRemove->setFixedWidth(btnSize);
        btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));

        QToolButton* btnCopy = new QToolButton(buttonsColumnWidget);
        btnCopy->setProperty("row", row);
        btnCopy->setFixedSize(btnSize, btnSize);
        btnCopy->setIcon(QIcon(":/img/actions/copy.png"));
        btnCopy->setIconSize(iconSize);
        connect(btnCopy, &QToolButton::clicked,
                this, &FootprintListEditorWidget::btnCopyClicked);
        buttonsColumnLayout->addWidget(btnCopy);

        QToolButton* btnUp = new QToolButton(buttonsColumnWidget);
        btnUp->setProperty("row", row);
        btnUp->setFixedSize(btnSize, btnSize);
        btnUp->setIcon(QIcon(":/img/actions/up.png"));
        btnUp->setIconSize(iconSize);
        btnUp->setEnabled(rowToIndex(row) > 0);
        connect(btnUp, &QToolButton::clicked,
                this, &FootprintListEditorWidget::btnUpClicked);
        buttonsColumnLayout->addWidget(btnUp);

        QToolButton* btnDown = new QToolButton(buttonsColumnWidget);
        btnDown->setProperty("row", row);
        btnDown->setFixedSize(btnSize, btnSize);
        btnDown->setIcon(QIcon(":/img/actions/down.png"));
        btnDown->setIconSize(iconSize);
        btnDown->setEnabled(rowToIndex(row) < mFootprintList->count() - 1);
        connect(btnDown, &QToolButton::clicked,
                this, &FootprintListEditorWidget::btnDownClicked);
        buttonsColumnLayout->addWidget(btnDown);
    } else {
        btnAddRemove->setFixedWidth(btnSize * 4);
        btnAddRemove->setIcon(QIcon(":/img/actions/add.png"));
    }
    buttonsColumnLayout->addWidget(btnAddRemove);
    mTable->setCellWidget(row, COLUMN_BUTTONS, buttonsColumnWidget);

    // adjust the height of the row according to the size of the contained widgets
    mTable->verticalHeader()->resizeSection(row, btnSize);
}

void FootprintListEditorWidget::addFootprint(const QString& name) noexcept
{
    try {
        throwIfNameEmptyOrExists(name);
        Uuid uuid = Uuid::createRandom();
        mUndoStack->execCmd(new CmdFootprintInsert(*mFootprintList,
            std::make_shared<Footprint>(uuid, name, ""))); // can throw
        updateTable(uuid);
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not add footprint"), e.getMsg());
    }
}

void FootprintListEditorWidget::removeFootprint(const Uuid& uuid) noexcept
{
    try {
        const Footprint* footprint = mFootprintList->get(uuid).get(); // can throw
        mUndoStack->execCmd(new CmdFootprintRemove(*mFootprintList, footprint)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not remove footprint"), e.getMsg());
    }
}

void FootprintListEditorWidget::moveFootprintUp(int index) noexcept
{
    Q_ASSERT(index >= 1 && index < mFootprintList->count());
    try {
        mUndoStack->execCmd(new CmdFootprintsSwap(*mFootprintList, index, index - 1)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not move footprint"), e.getMsg());
    }
}

void FootprintListEditorWidget::moveFootprintDown(int index) noexcept
{
    Q_ASSERT(index >= 0 && index < mFootprintList->count() - 1);
    try {
        mUndoStack->execCmd(new CmdFootprintsSwap(*mFootprintList, index, index + 1)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not move footprint"), e.getMsg());
    }
}

void FootprintListEditorWidget::copyFootprint(const Uuid& uuid) noexcept
{
    try {
        const Footprint* original = mFootprintList->get(uuid).get(); // can throw
        std::shared_ptr<Footprint> copy(new Footprint(Uuid::createRandom(),
                                        "Copy of " % original->getNames().getDefaultValue(), "")); // can throw
        copy->getDescriptions() = original->getDescriptions();
        copy->getPads() = original->getPads();
        copy->getPolygons() = original->getPolygons();
        copy->getEllipses() = original->getEllipses();
        copy->getStrokeTexts() = original->getStrokeTexts();
        copy->getHoles() = original->getHoles();
        mUndoStack->execCmd(new CmdFootprintInsert(*mFootprintList, copy)); // can throw
        updateTable(copy->getUuid());
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not copy footprint"), e.getMsg());
    }
}

QString FootprintListEditorWidget::setName(const Uuid& uuid, const QString& name) noexcept
{
    Footprint* footprint = mFootprintList->find(uuid).get(); Q_ASSERT(footprint);
    if (footprint->getNames().getDefaultValue() == name) {
        return footprint->getNames().getDefaultValue();
    }

    try {
        throwIfNameEmptyOrExists(name);
        QScopedPointer<CmdFootprintEdit> cmd(new CmdFootprintEdit(*footprint));
        cmd->setName(name);
        mUndoStack->execCmd(cmd.take());
        return name;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Invalid name"), e.getMsg());
        return footprint->getNames().getDefaultValue();
    }
}

int FootprintListEditorWidget::getRowOfTableCellWidget(QObject* obj) const noexcept
{
    bool ok = false;
    int row = obj->property("row").toInt(&ok); Q_ASSERT(ok);
    Q_ASSERT(row >= 0 && row < mTable->rowCount());
    return row;
}

Uuid FootprintListEditorWidget::getUuidOfRow(int row) const noexcept
{
    if (isExistingFootprintRow(row)) {
        return mFootprintList->value(rowToIndex(row))->getUuid();
    } else {
        return Uuid();
    }
}

void FootprintListEditorWidget::throwIfNameEmptyOrExists(const QString& name) const
{
    if (name.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__, tr("The name must not be empty."));
    }
    for (const Footprint& footprint : *mFootprintList) {
        if (footprint.getNames().getDefaultValue() == name) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("There is already a footprint with the name \"%1\".")).arg(name));
        }
    }
}

QString FootprintListEditorWidget::cleanName(const QString& name) noexcept
{
    // TODO: it's ugly to use a method from FilePath...
    return FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::KeepCase);
}

/*****************************************************************************************
 *  Observer Methods
 ****************************************************************************************/

void FootprintListEditorWidget::listObjectAdded(const FootprintList& list, int newIndex,
                                                const std::shared_ptr<Footprint>& ptr) noexcept
{
    Q_ASSERT(&list == mFootprintList); Q_UNUSED(list); Q_UNUSED(newIndex); Q_UNUSED(ptr);
    updateTable(mSelectedFootprint);
}

void FootprintListEditorWidget::listObjectRemoved(const FootprintList& list, int oldIndex,
                                                  const std::shared_ptr<Footprint>& ptr) noexcept
{
    Q_ASSERT(&list == mFootprintList); Q_UNUSED(list); Q_UNUSED(oldIndex); Q_UNUSED(ptr);
    updateTable(mSelectedFootprint);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
