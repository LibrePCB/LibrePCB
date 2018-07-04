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
#include "componentsymbolvariantlistwidget.h"
#include <librepcb/common/undostack.h>
#include <librepcb/library/cmp/cmd/cmdcomponentsymbolvariantedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSymbolVariantListWidget::ComponentSymbolVariantListWidget(QWidget* parent) noexcept :
    QWidget(parent), mTable(new QTableWidget(this)), mUndoStack(nullptr),
    mVariantList(nullptr), mEditorProvider(nullptr)
{
    mTable->setCornerButtonEnabled(false);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setColumnCount(_COLUMN_COUNT);
    mTable->setHorizontalHeaderItem(COLUMN_UUID,        new QTableWidgetItem(tr("UUID")));
    mTable->setHorizontalHeaderItem(COLUMN_NAME,        new QTableWidgetItem(tr("Name")));
    mTable->setHorizontalHeaderItem(COLUMN_DESCRIPTION, new QTableWidgetItem(tr("Description")));
    mTable->setHorizontalHeaderItem(COLUMN_NORM,        new QTableWidgetItem(tr("Norm")));
    mTable->setHorizontalHeaderItem(COLUMN_SYMBOLCOUNT, new QTableWidgetItem(tr("Symbols")));
    mTable->setHorizontalHeaderItem(COLUMN_BUTTONS,     new QTableWidgetItem(tr("Actions")));
    mTable->horizontalHeaderItem(COLUMN_SYMBOLCOUNT)->setTextAlignment(Qt::AlignCenter);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_UUID,        QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_NAME,        QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_DESCRIPTION, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_NORM,        QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_SYMBOLCOUNT, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_BUTTONS,     QHeaderView::ResizeToContents);
    mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mTable->verticalHeader()->setMinimumSectionSize(20);
    mTable->verticalHeader()->setVisible(false);
    mSortAsc = true;
    QHeaderView *header = mTable->horizontalHeader();
    connect(header, &QHeaderView::sectionClicked,
            this, &ComponentSymbolVariantListWidget::HeaderClicked);
    connect(mTable, &QTableWidget::currentCellChanged,
            this, &ComponentSymbolVariantListWidget::currentCellChanged);
    connect(mTable, &QTableWidget::cellDoubleClicked,
            this, &ComponentSymbolVariantListWidget::cellDoubleClicked);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mTable);

    updateTable();
}

ComponentSymbolVariantListWidget::~ComponentSymbolVariantListWidget() noexcept
{
    setReferences(nullptr, nullptr, nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentSymbolVariantListWidget::setReferences(UndoStack* undoStack,
    ComponentSymbolVariantList* variants, IF_ComponentSymbolVariantEditorProvider* editorProvider) noexcept
{
    if (mVariantList) {
        mVariantList->unregisterObserver(this);
        for (const ComponentSymbolVariant& variant : *mVariantList) {
            disconnect(&variant, &ComponentSymbolVariant::edited,
                       this, &ComponentSymbolVariantListWidget::updateTable);
        }
    }
    mUndoStack = undoStack;
    mVariantList = variants;
    mEditorProvider = editorProvider;
    mSelectedVariant = Uuid();
    if (mVariantList) {
        mVariantList->registerObserver(this);
        for (const ComponentSymbolVariant& variant : *mVariantList) {
            connect(&variant, &ComponentSymbolVariant::edited,
                    this, &ComponentSymbolVariantListWidget::updateTable);
        }
    }
    updateTable();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void ComponentSymbolVariantListWidget::HeaderClicked(int ind){
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

void ComponentSymbolVariantListWidget::currentCellChanged(int currentRow, int currentColumn,
                                                          int previousRow, int previousColumn) noexcept
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    if (!allReferencesValid()) return;
    mSelectedVariant = getUuidOfRow(currentRow);
}

void ComponentSymbolVariantListWidget::cellDoubleClicked(int row, int column) noexcept
{
    Q_UNUSED(column);
    if (!allReferencesValid()) return;
    if (isExistingVariantRow(row)) {
        editVariant(getUuidOfRow(row));
    }
}

void ComponentSymbolVariantListWidget::btnEditClicked() noexcept
{
    if (!allReferencesValid()) return;
    int row = getRowOfTableCellWidget(sender());
    if (isExistingVariantRow(row)) {
        editVariant(getUuidOfRow(row));
    }
}

void ComponentSymbolVariantListWidget::btnAddRemoveClicked() noexcept
{
    if (!allReferencesValid()) return;
    int row = getRowOfTableCellWidget(sender());
    if (isNewVariantRow(row)) {
        const QTableWidgetItem* nameItem = mTable->item(row, COLUMN_NAME); Q_ASSERT(nameItem);
        const QTableWidgetItem* descItem = mTable->item(row, COLUMN_DESCRIPTION); Q_ASSERT(descItem);
        const QTableWidgetItem* normItem = mTable->item(row, COLUMN_NORM); Q_ASSERT(normItem);
        addVariant(nameItem->text().trimmed(), descItem->text().trimmed(), normItem->text().trimmed());
    } else if (isExistingVariantRow(row)) {
        removeVariant(getUuidOfRow(row));
    }
}

void ComponentSymbolVariantListWidget::btnUpClicked() noexcept
{
    if (!allReferencesValid()) return;
    int row = getRowOfTableCellWidget(sender());
    if (!isExistingVariantRow(row)) return;
    int index = rowToIndex(row);
    if (index <= 0) return;
    moveVariantUp(index);
}

void ComponentSymbolVariantListWidget::btnDownClicked() noexcept
{
    if (!allReferencesValid()) return;
    int row = getRowOfTableCellWidget(sender());
    if (!isExistingVariantRow(row)) return;
    int index = rowToIndex(row);
    if (index >= mVariantList->count() - 1) return;
    moveVariantDown(index);
}

/*****************************************************************************************
 *  Observer
 ****************************************************************************************/

void ComponentSymbolVariantListWidget::listObjectAdded(const ComponentSymbolVariantList& list,
    int newIndex, const std::shared_ptr<ComponentSymbolVariant>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == mVariantList);
    connect(ptr.get(), &ComponentSymbolVariant::edited,
            this, &ComponentSymbolVariantListWidget::updateTable);
    updateTable();
}

void ComponentSymbolVariantListWidget::listObjectRemoved(const ComponentSymbolVariantList& list,
    int oldIndex, const std::shared_ptr<ComponentSymbolVariant>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == mVariantList);
    disconnect(ptr.get(), &ComponentSymbolVariant::edited,
               this, &ComponentSymbolVariantListWidget::updateTable);
    updateTable();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ComponentSymbolVariantListWidget::updateTable() noexcept
{
    blockSignals(true);

    // remove all rows
    mTable->clearSelection();
    mTable->clearContents();

    if (allReferencesValid()) {
        int selectedRow = newVariantRow();
        mTable->setEnabled(true);
        mTable->setRowCount(mVariantList->count() + 1);

        // special row for adding a new symbol variant
        setTableRowContent(newVariantRow(), Uuid(), "", "", "", 0);

        // existing symbol variants
        for (int i = 0; i < mVariantList->count(); ++i) {
            const ComponentSymbolVariant& variant = *mVariantList->at(i);
            setTableRowContent(indexToRow(i), variant.getUuid(),
                               variant.getNames().getDefaultValue(),
                               variant.getDescriptions().getDefaultValue(),
                               variant.getNorm(), variant.getSymbolItems().count());
            if (variant.getUuid() == mSelectedVariant) {
                selectedRow = indexToRow(i);
            }
        }

        // workaround to trigger column resizing because sometimes auto-resizing does not work
        mTable->hide(); mTable->show();

        // set selected row
        mTable->selectRow(selectedRow);
        mSelectedVariant = getUuidOfRow(selectedRow);
    } else {
        mTable->setEnabled(false);
    }

    blockSignals(false);
}

void ComponentSymbolVariantListWidget::setTableRowContent(int row, const Uuid& uuid,
    const QString& name, const QString& desc, const QString& norm, int symbolCount) noexcept
{
    // header
    QString header = uuid.isNull() ? tr("Add new variant:") : uuid.toStr().left(13) % "...";
    QTableWidgetItem* headerItem = new QTableWidgetItem(header);
    headerItem->setToolTip(uuid.toStr());
    QFont headerFont = headerItem->font();
    headerFont.setStyleHint(QFont::Monospace); // ensure that the column width is fixed
    headerFont.setFamily("Monospace");
    headerItem->setFont(headerFont);
    //mTable->setVerticalHeaderItem(row, headerItem);
    headerItem->setFlags(headerItem->flags() ^ Qt::ItemIsEditable);
    QPalette pallet = mTable->verticalHeader()->palette();
    headerItem->setBackgroundColor(pallet.background().color());
    mTable->setItem(row, COLUMN_UUID, headerItem);

    // name
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);
    if (row != newVariantRow()) {
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
    }
    mTable->setItem(row, COLUMN_NAME, nameItem);

    // description
    QTableWidgetItem* descItem = new QTableWidgetItem(desc);
    if (row != newVariantRow()) {
        descItem->setFlags(descItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
    }
    mTable->setItem(row, COLUMN_DESCRIPTION, descItem);

    // norm
    QTableWidgetItem* normItem = new QTableWidgetItem(norm);
    if (row != newVariantRow()) {
        normItem->setFlags(normItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
    }
    mTable->setItem(row, COLUMN_NORM, normItem);

    // symbol count
    QTableWidgetItem* symbolCountItem = new QTableWidgetItem(QString::number(symbolCount));
    symbolCountItem->setFlags(symbolCountItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
    symbolCountItem->setTextAlignment(Qt::AlignCenter);
    if ((symbolCount < 1) && isExistingVariantRow(row)) {
        symbolCountItem->setBackgroundColor(Qt::red);
        symbolCountItem->setTextColor(Qt::white);
    }
    mTable->setItem(row, COLUMN_SYMBOLCOUNT, symbolCountItem);

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
            this, &ComponentSymbolVariantListWidget::btnAddRemoveClicked);
    if (isExistingVariantRow(row)) {
        btnAddRemove->setFixedWidth(btnSize);
        btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));

        QToolButton* btnEdit = new QToolButton(buttonsColumnWidget);
        btnEdit->setProperty("row", row);
        btnEdit->setFixedSize(btnSize, btnSize);
        btnEdit->setIcon(QIcon(":/img/actions/edit.png"));
        btnEdit->setIconSize(iconSize);
        connect(btnEdit, &QToolButton::clicked,
                this, &ComponentSymbolVariantListWidget::btnEditClicked);
        buttonsColumnLayout->addWidget(btnEdit);

        QToolButton* btnUp = new QToolButton(buttonsColumnWidget);
        btnUp->setProperty("row", row);
        btnUp->setFixedSize(btnSize, btnSize);
        btnUp->setIcon(QIcon(":/img/actions/up.png"));
        btnUp->setIconSize(iconSize);
        btnUp->setEnabled(rowToIndex(row) > 0);
        connect(btnUp, &QToolButton::clicked,
                this, &ComponentSymbolVariantListWidget::btnUpClicked);
        buttonsColumnLayout->addWidget(btnUp);

        QToolButton* btnDown = new QToolButton(buttonsColumnWidget);
        btnDown->setProperty("row", row);
        btnDown->setFixedSize(btnSize, btnSize);
        btnDown->setIcon(QIcon(":/img/actions/down.png"));
        btnDown->setIconSize(iconSize);
        btnDown->setEnabled(rowToIndex(row) < mVariantList->count() - 1);
        connect(btnDown, &QToolButton::clicked,
                this, &ComponentSymbolVariantListWidget::btnDownClicked);
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

void ComponentSymbolVariantListWidget::addVariant(const QString& name, const QString& desc,
    const QString& norm) noexcept
{
    try {
        if (name.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The name must not be empty.")).arg(name));
        }
        std::shared_ptr<ComponentSymbolVariant> variant(
            new ComponentSymbolVariant(Uuid::createRandom(), norm, name, desc)); // can throw
        mUndoStack->execCmd(new CmdComponentSymbolVariantInsert(*mVariantList, variant)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not add symbol variant"), e.getMsg());
    }
}

void ComponentSymbolVariantListWidget::removeVariant(const Uuid& uuid) noexcept
{
    try {
        const ComponentSymbolVariant* variant = mVariantList->get(uuid).get(); // can throw
        mUndoStack->execCmd(new CmdComponentSymbolVariantRemove(*mVariantList, variant)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not remove symbol variant"), e.getMsg());
    }
}

void ComponentSymbolVariantListWidget::moveVariantUp(int index) noexcept
{
    Q_ASSERT(index >= 1 && index < mVariantList->count());
    try {
        mUndoStack->execCmd(new CmdComponentSymbolVariantsSwap(*mVariantList, index, index - 1)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not move symbol variant"), e.getMsg());
    }
}

void ComponentSymbolVariantListWidget::moveVariantDown(int index) noexcept
{
    Q_ASSERT(index >= 0 && index < mVariantList->count() - 1);
    try {
        mUndoStack->execCmd(new CmdComponentSymbolVariantsSwap(*mVariantList, index, index + 1)); // can throw
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not move symbol variant"), e.getMsg());
    }
}

void ComponentSymbolVariantListWidget::editVariant(const Uuid& uuid) noexcept
{
    try {
        ComponentSymbolVariant* variant = mVariantList->get(uuid).get(); // can throw
        ComponentSymbolVariant copy(*variant);
        if (mEditorProvider->openComponentSymbolVariantEditor(copy)) {
            QScopedPointer<CmdComponentSymbolVariantEdit> cmd(
                new CmdComponentSymbolVariantEdit(*variant));
            cmd->setNorm(copy.getNorm());
            cmd->setNames(copy.getNames());
            cmd->setDescriptions(copy.getDescriptions());
            cmd->setSymbolItems(copy.getSymbolItems());
            mUndoStack->execCmd(cmd.take());
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit symbol variant"), e.getMsg());
    }
}

int ComponentSymbolVariantListWidget::getRowOfTableCellWidget(QObject* obj) const noexcept
{
    bool ok = false;
    int row = obj->property("row").toInt(&ok); Q_ASSERT(ok);
    Q_ASSERT(row >= 0 && row < mTable->rowCount());
    return row;
}

Uuid ComponentSymbolVariantListWidget::getUuidOfRow(int row) const noexcept
{
    if (isExistingVariantRow(row)) {
        return mVariantList->at(rowToIndex(row))->getUuid();
    } else {
        return Uuid();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
