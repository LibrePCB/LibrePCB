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
#include "componentsignallisteditorwidget.h"
#include <librepcb/common/undostack.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/widgets/signalrolecombobox.h>
#include <librepcb/common/widgets/centeredcheckbox.h>
#include <librepcb/library/cmp/cmd/cmdcomponentsignaledit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSignalListEditorWidget::ComponentSignalListEditorWidget(QWidget* parent) noexcept :
    QWidget(parent), mTable(new QTableWidget(this)),
    mUndoStack(nullptr), mSignalList(nullptr)
{
    mTable->setCornerButtonEnabled(false);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setColumnCount(_COLUMN_COUNT);
    mTable->setHorizontalHeaderItem(COLUMN_UUID,          new QTableWidgetItem(tr("UUID")));
    mTable->setHorizontalHeaderItem(COLUMN_NAME,          new QTableWidgetItem(tr("Pin Name")));
    //mTable->setHorizontalHeaderItem(COLUMN_ROLE,          new QTableWidgetItem(tr("Role (ERC)")));
    mTable->setHorizontalHeaderItem(COLUMN_ISREQUIRED,    new QTableWidgetItem(tr("Required")));
    //mTable->setHorizontalHeaderItem(COLUMN_ISNEGATED,     new QTableWidgetItem(tr("Negated")));
    //mTable->setHorizontalHeaderItem(COLUMN_ISCLOCK,       new QTableWidgetItem(tr("Clock")));
    mTable->setHorizontalHeaderItem(COLUMN_FORCEDNETNAME, new QTableWidgetItem(tr("Forced Net")));
    mTable->setHorizontalHeaderItem(COLUMN_BUTTONS,       new QTableWidgetItem(""));
    mTable->horizontalHeaderItem(COLUMN_ISREQUIRED)->setTextAlignment(Qt::AlignCenter);
    //mTable->horizontalHeaderItem(COLUMN_ISNEGATED)->setTextAlignment(Qt::AlignCenter);
    //mTable->horizontalHeaderItem(COLUMN_ISCLOCK)->setTextAlignment(Qt::AlignCenter);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_UUID,          QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_NAME,          QHeaderView::Stretch);
    //mTable->horizontalHeader()->setSectionResizeMode(COLUMN_ROLE,          QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_ISREQUIRED,    QHeaderView::ResizeToContents);
    //mTable->horizontalHeader()->setSectionResizeMode(COLUMN_ISNEGATED,     QHeaderView::ResizeToContents);
    //mTable->horizontalHeader()->setSectionResizeMode(COLUMN_ISCLOCK,       QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_FORCEDNETNAME, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_BUTTONS,       QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setMinimumSectionSize(10);
//    mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
//    mTable->verticalHeader()->setMinimumSectionSize(20);
    mTable->verticalHeader()->setVisible(false);
    mSortAsc = true;
    QHeaderView *header = mTable->horizontalHeader();
    connect(header, &QHeaderView::sectionClicked,
            this, &ComponentSignalListEditorWidget::HeaderClicked);
    connect(mTable, &QTableWidget::currentCellChanged,
            this, &ComponentSignalListEditorWidget::currentCellChanged);
    connect(mTable, &QTableWidget::cellChanged,
            this, &ComponentSignalListEditorWidget::tableCellChanged);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mTable);

    updateTable();
}

ComponentSignalListEditorWidget::~ComponentSignalListEditorWidget() noexcept
{
    setReferences(nullptr, nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentSignalListEditorWidget::setReferences(UndoStack* undoStack,
                                                    ComponentSignalList* list) noexcept
{
    if (mSignalList) {
        mSignalList->unregisterObserver(this);
        for (const ComponentSignal& signal : *mSignalList) {
            disconnect(&signal, &ComponentSignal::edited,
                       this, &ComponentSignalListEditorWidget::updateTable);
        }
    }
    mUndoStack = undoStack;
    mSignalList = list;
    mSelectedSignal = Uuid();
    if (mSignalList) {
        mSignalList->registerObserver(this);
        for (const ComponentSignal& signal : *mSignalList) {
            connect(&signal, &ComponentSignal::edited,
                    this, &ComponentSignalListEditorWidget::updateTable);
        }
    }
    updateTable();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void ComponentSignalListEditorWidget::HeaderClicked(int ind){
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

void ComponentSignalListEditorWidget::currentCellChanged(int currentRow, int currentColumn,
                                                         int previousRow, int previousColumn) noexcept
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    if (!mSignalList) return;
    mSelectedSignal = getUuidOfRow(currentRow);
}

void ComponentSignalListEditorWidget::tableCellChanged(int row, int column) noexcept
{
    if (!mSignalList) return;
    QTableWidgetItem* item = mTable->item(row, column); Q_ASSERT(item);

    if (isNewSignalRow(row)) {
        if (column == COLUMN_NAME) {
            item->setText(cleanName(item->text()));
        } else if (column == COLUMN_FORCEDNETNAME) {
            item->setText(cleanForcedNetName(item->text()));
        }
    } else if (isExistingSignalRow(row)) {
        if (column == COLUMN_NAME) {
            setName(getUuidOfRow(row), cleanName(item->text()));
        } else if (column == COLUMN_FORCEDNETNAME) {
            item->setText(cleanForcedNetName(item->text()));
            setForcedNetName(getUuidOfRow(row), cleanForcedNetName(item->text()));
        }
    }
}

/*void ComponentSignalListEditorWidget::signalRoleChanged(const SignalRole& role) noexcept
{
    if (!mSignalList) return;
    int row = getRowOfTableCellWidget(sender());
    if (isExistingSignalRow(row)) {
        setRole(getUuidOfRow(row), role);
    }
}*/

void ComponentSignalListEditorWidget::isRequiredChanged(bool checked) noexcept
{
    if (!mSignalList) return;
    int row = getRowOfTableCellWidget(sender());
    if (isExistingSignalRow(row)) {
        setIsRequired(getUuidOfRow(row), checked);
    }
}

/*void ComponentSignalListEditorWidget::isNegatedChanged(bool checked) noexcept
{
    if (!mSignalList) return;
    int row = getRowOfTableCellWidget(sender());
    if (isExistingSignalRow(row)) {
        setIsNegated(getUuidOfRow(row), checked);
    }
}

void ComponentSignalListEditorWidget::isClockChanged(bool checked) noexcept
{
    if (!mSignalList) return;
    int row = getRowOfTableCellWidget(sender());
    if (isExistingSignalRow(row)) {
        setIsClock(getUuidOfRow(row), checked);
    }
}*/

void ComponentSignalListEditorWidget::btnAddRemoveClicked() noexcept
{
    if (!mSignalList) return;
    int row = getRowOfTableCellWidget(sender());
    if (isNewSignalRow(row)) {
        const QTableWidgetItem* nameItem = mTable->item(row, COLUMN_NAME); Q_ASSERT(nameItem);
        //const SignalRoleComboBox* roleComboBox = dynamic_cast<const SignalRoleComboBox*>
        //    (mTable->cellWidget(row, COLUMN_ROLE)); Q_ASSERT(roleComboBox);
        const CenteredCheckBox* requiredCheckBox = dynamic_cast<const CenteredCheckBox*>
            (mTable->cellWidget(row, COLUMN_ISREQUIRED)); Q_ASSERT(requiredCheckBox);
        //const CenteredCheckBox* negatedCheckBox = dynamic_cast<const CenteredCheckBox*>
        //    (mTable->cellWidget(row, COLUMN_ISNEGATED)); Q_ASSERT(negatedCheckBox);
        //const CenteredCheckBox* clockCheckBox = dynamic_cast<const CenteredCheckBox*>
        //    (mTable->cellWidget(row, COLUMN_ISCLOCK)); Q_ASSERT(clockCheckBox);
        const QTableWidgetItem* netNameItem = mTable->item(row, COLUMN_FORCEDNETNAME); Q_ASSERT(netNameItem);
        addSignal(cleanName(nameItem->text()), /*roleComboBox->getCurrentItem(),*/
                  requiredCheckBox->isChecked(),
                  //negatedCheckBox->isChecked(),
                  //clockCheckBox->isChecked(),
                  cleanForcedNetName(netNameItem->text()));
    } else if (isExistingSignalRow(row)) {
        removeSignal(getUuidOfRow(row));
    }
}

/*****************************************************************************************
 *  Observer
 ****************************************************************************************/

void ComponentSignalListEditorWidget::listObjectAdded(const ComponentSignalList& list,
    int newIndex, const std::shared_ptr<ComponentSignal>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == mSignalList);
    connect(ptr.get(), &ComponentSignal::edited,
            this, &ComponentSignalListEditorWidget::updateTable);
    updateTable();
}

void ComponentSignalListEditorWidget::listObjectRemoved(const ComponentSignalList& list,
    int oldIndex, const std::shared_ptr<ComponentSignal>& ptr) noexcept
{
    Q_UNUSED(list);
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == mSignalList);
    disconnect(ptr.get(), &ComponentSignal::edited,
               this, &ComponentSignalListEditorWidget::updateTable);
    updateTable();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ComponentSignalListEditorWidget::updateTable() noexcept
{
    mTable->blockSignals(true);

    // remove all rows
    mTable->clearSelection();
    mTable->clearContents();

    if (mSignalList) {
        mSignalList->sortedByName(mSortAsc);

        int selectedRow = newSignalRow();
        mTable->setEnabled(true);
        mTable->setRowCount(mSignalList->count() + 1);

        // special row for adding a new signal
        setTableRowContent(newSignalRow(), Uuid(), "", /*SignalRole::passive(),*/ false,
                           /*false, false,*/ "");
        //std::sort(mSignalList->mObjects.begin(), mSignalList->mObjects.end());//, wayToSort);

        // existing signals
        for (int i = 0; i < mSignalList->count(); ++i) {
            const ComponentSignal& signal = *mSignalList->at(i);
            setTableRowContent(indexToRow(i), signal.getUuid(), signal.getName(),
                               /*signal.getRole(),*/ signal.isRequired(), /*signal.isNegated(),
                               signal.isClock(),*/ signal.getForcedNetName());
            if (signal.getUuid() == mSelectedSignal) {
                selectedRow = indexToRow(i);
            }
        }

        // workaround to trigger column resizing because sometimes auto-resizing does not work
        mTable->hide(); mTable->show();

        // set selected row
        mTable->selectRow(selectedRow);
        mSelectedSignal = getUuidOfRow(selectedRow);
    } else {
        mTable->setEnabled(false);
    }

    mTable->blockSignals(false);
}

void ComponentSignalListEditorWidget::setTableRowContent(int row, const Uuid& uuid,
    const QString& name, /*const SignalRole& role,*/ bool required, /*bool negated,
    bool clock,*/ const QString& forcedNetName) noexcept
{
    // header
    QString header = uuid.isNull() ? tr("Add new signal:") : uuid.toStr().left(13) % "...";
//    QTableWidgetItem* headerItem = new QTableWidgetItem(header);
//    headerItem->setToolTip(uuid.toStr());
//    QFont headerFont = headerItem->font();
//    headerFont.setStyleHint(QFont::Monospace); // ensure that the column width is fixed
//    headerFont.setFamily("Monospace");
//    headerItem->setFont(headerFont);
//    mTable->setVerticalHeaderItem(row, headerItem);
    QTableWidgetItem *headerItem = new QTableWidgetItem(header);
    headerItem->setToolTip(uuid.toStr());
    QFont headerFont = headerItem->font();
    headerFont.setStyleHint(QFont::Monospace); // ensure that the column width is fixed
    headerFont.setFamily("Monospace");
    headerItem->setFont(headerFont);
    headerItem->setFlags(headerItem->flags() ^ Qt::ItemIsEditable);
    QPalette pallet = mTable->verticalHeader()->palette();
    headerItem->setBackgroundColor(pallet.background().color());
    mTable->setItem(row, COLUMN_UUID, headerItem);

    // name
    mTable->setItem(row, COLUMN_NAME, new QTableWidgetItem(name));

    // role
    //SignalRoleComboBox* roleComboBox = new SignalRoleComboBox(this);
    //roleComboBox->setProperty("row", row);
    //roleComboBox->setStyleSheet("padding: 0px 3px 0px 3px;"); // reduce required space
    //roleComboBox->setCurrentItem(role);
    //connect(roleComboBox, &SignalRoleComboBox::currentItemChanged,
    //        this, &ComponentSignalListEditorWidget::signalRoleChanged);
    //mTable->setCellWidget(row, COLUMN_ROLE, roleComboBox);

    // required
    CenteredCheckBox* requiredCheckBox = new CenteredCheckBox(this);
    requiredCheckBox->setProperty("row", row);
    requiredCheckBox->setChecked(required);
    connect(requiredCheckBox, &CenteredCheckBox::toggled,
            this, &ComponentSignalListEditorWidget::isRequiredChanged);
    mTable->setCellWidget(row, COLUMN_ISREQUIRED, requiredCheckBox);

    // negated
    //CenteredCheckBox* negatedCheckBox = new CenteredCheckBox(this);
    //negatedCheckBox->setProperty("row", row);
    //negatedCheckBox->setChecked(negated);
    //connect(negatedCheckBox, &CenteredCheckBox::toggled,
    //        this, &ComponentSignalListEditorWidget::isNegatedChanged);
    //mTable->setCellWidget(row, COLUMN_ISNEGATED, negatedCheckBox);

    // clock
    //CenteredCheckBox* clockCheckBox = new CenteredCheckBox(this);
    //clockCheckBox->setProperty("row", row);
    //clockCheckBox->setChecked(clock);
    //connect(clockCheckBox, &CenteredCheckBox::toggled,
    //        this, &ComponentSignalListEditorWidget::isClockChanged);
    //mTable->setCellWidget(row, COLUMN_ISCLOCK, clockCheckBox);

    // forced net name
    mTable->setItem(row, COLUMN_FORCEDNETNAME, new QTableWidgetItem(forcedNetName));

    // button
    int btnSize = requiredCheckBox->sizeHint().height();
    QToolButton* btnAddRemove = new QToolButton(this);
    btnAddRemove->setProperty("row", row);
    btnAddRemove->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnAddRemove->setFixedSize(btnSize, btnSize);
    btnAddRemove->setIconSize(QSize(btnSize - 6, btnSize - 6));
    connect(btnAddRemove, &QToolButton::clicked,
            this, &ComponentSignalListEditorWidget::btnAddRemoveClicked);
    if (isExistingSignalRow(row)) {
        btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));
    } else {
        btnAddRemove->setIcon(QIcon(":/img/actions/add.png"));
    }
    mTable->setCellWidget(row, COLUMN_BUTTONS, btnAddRemove);

    // adjust the height of the row according to the size of the contained widgets
    mTable->verticalHeader()->resizeSection(row, btnSize);
}

void ComponentSignalListEditorWidget::addSignal(const QString& name, /*const SignalRole& role,*/
    bool required, /*bool negated, bool clock,*/ const QString& forcedNetName) noexcept
{
    try {
        throwIfNameEmptyOrExists(name);
        std::shared_ptr<ComponentSignal> signal(new ComponentSignal(Uuid::createRandom(), name)); // can throw
        //signal->setRole(role);
        signal->setIsRequired(required);
        //signal->setIsNegated(negated);
        //signal->setIsClock(clock);
        signal->setForcedNetName(forcedNetName);
        if (mUndoStack) {
            mUndoStack->execCmd(new CmdComponentSignalInsert(*mSignalList, signal));
        } else {
            mSignalList->append(signal);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not add signal"), e.getMsg());
    }
}

void ComponentSignalListEditorWidget::removeSignal(const Uuid& uuid) noexcept
{
    try {
        if (mUndoStack) {
            const ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
            mUndoStack->execCmd(new CmdComponentSignalRemove(*mSignalList, signal));
        } else {
            mSignalList->remove(uuid);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not remove signal"), e.getMsg());
    }
}

bool ComponentSignalListEditorWidget::setName(const Uuid& uuid, const QString& name) noexcept
{
    try {
        ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
        if (signal->getName() == name) return true;
        throwIfNameEmptyOrExists(name);
        if (mUndoStack) {
            QScopedPointer<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*signal));
            cmd->setName(name);
            mUndoStack->execCmd(cmd.take());
        } else {
            signal->setName(name);
        }
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit signal"), e.getMsg());
        return false;
    }
}

/*void ComponentSignalListEditorWidget::setRole(const Uuid& uuid, const SignalRole& role) noexcept
{
    try {
        ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
        if (signal->getRole() == role) return;
        if (mUndoStack) {
            QScopedPointer<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*signal));
            cmd->setRole(role);
            mUndoStack->execCmd(cmd.take());
        } else {
            signal->setRole(role);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit signal"), e.getMsg());
    }
}*/

void ComponentSignalListEditorWidget::setIsRequired(const Uuid& uuid, bool required) noexcept
{
    try {
        ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
        if (signal->isRequired() == required) return;
        if (mUndoStack) {
            QScopedPointer<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*signal));
            cmd->setIsRequired(required);
            mUndoStack->execCmd(cmd.take());
        } else {
            signal->setIsRequired(required);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit signal"), e.getMsg());
    }
}

/*void ComponentSignalListEditorWidget::setIsNegated(const Uuid& uuid, bool negated) noexcept
{
    try {
        ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
        if (signal->isNegated() == negated) return;
        if (mUndoStack) {
            QScopedPointer<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*signal));
            cmd->setIsNegated(negated);
            mUndoStack->execCmd(cmd.take());
        } else {
            signal->setIsNegated(negated);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit signal"), e.getMsg());
    }
}

void ComponentSignalListEditorWidget::setIsClock(const Uuid& uuid, bool clock) noexcept
{
    try {
        ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
        if (signal->isClock() == clock) return;
        if (mUndoStack) {
            QScopedPointer<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*signal));
            cmd->setIsClock(clock);
            mUndoStack->execCmd(cmd.take());
        } else {
            signal->setIsClock(clock);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit signal"), e.getMsg());
    }
}*/

void ComponentSignalListEditorWidget::setForcedNetName(const Uuid& uuid, const QString& netname) noexcept
{
    try {
        ComponentSignal* signal = mSignalList->get(uuid).get(); // can throw
        if (signal->getForcedNetName() == netname) return;
        if (mUndoStack) {
            QScopedPointer<CmdComponentSignalEdit> cmd(new CmdComponentSignalEdit(*signal));
            cmd->setForcedNetName(netname);
            mUndoStack->execCmd(cmd.take());
        } else {
            signal->setForcedNetName(netname);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not edit signal"), e.getMsg());
    }
}

int ComponentSignalListEditorWidget::getRowOfTableCellWidget(QObject* obj) const noexcept
{
    bool ok = false;
    int row = obj->property("row").toInt(&ok); Q_ASSERT(ok);
    Q_ASSERT(row >= 0 && row < mTable->rowCount());
    return row;
}

Uuid ComponentSignalListEditorWidget::getUuidOfRow(int row) const noexcept
{
    if (isExistingSignalRow(row)) {
        return mSignalList->at(rowToIndex(row))->getUuid();
    } else {
        return Uuid();
    }
}

void ComponentSignalListEditorWidget::throwIfNameEmptyOrExists(const QString& name) const
{
    if (name.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__, tr("The name must not be empty."));
    }
    if (mSignalList->contains(name)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a signal with the name \"%1\".")).arg(name));
    }
}

QString ComponentSignalListEditorWidget::cleanName(const QString& name) noexcept
{
    // TODO: it's ugly to use a method from FilePath...
    return FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::KeepCase);
}

QString ComponentSignalListEditorWidget::cleanForcedNetName(const QString& name) noexcept
{
    // TODO: it's ugly to use a method from FilePath...
    return FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::KeepCase);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
