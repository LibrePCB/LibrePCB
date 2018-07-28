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
#include "attributelisteditorwidget.h"
#include "../attributes/attributetype.h"
#include "../attributes/attrtypestring.h"
#include "../attributes/attributeunit.h"
#include "attributetypecombobox.h"
#include "attributeunitcombobox.h"
#include "../fileio/filepath.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AttributeListEditorWidget::AttributeListEditorWidget(QWidget* parent) noexcept :
    QWidget(parent), mTable(new QTableWidget(this)), mSelectedAttribute(nullptr)
{
    mTable->setCornerButtonEnabled(false);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setColumnCount(_COLUMN_COUNT);
    mTable->setHorizontalHeaderItem(COLUMN_KEY,     new QTableWidgetItem(tr("Key")));
    mTable->setHorizontalHeaderItem(COLUMN_TYPE,    new QTableWidgetItem(tr("Type")));
    mTable->setHorizontalHeaderItem(COLUMN_VALUE,   new QTableWidgetItem(tr("Value")));
    mTable->setHorizontalHeaderItem(COLUMN_UNIT,    new QTableWidgetItem(tr("Unit")));
    mTable->setHorizontalHeaderItem(COLUMN_BUTTONS, new QTableWidgetItem(tr("Actions")));
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_KEY,     QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_TYPE,    QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_VALUE,   QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_UNIT,    QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(COLUMN_BUTTONS, QHeaderView::ResizeToContents);
    mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mTable->verticalHeader()->setMinimumSectionSize(20);
    connect(mTable, &QTableWidget::currentCellChanged,
            this, &AttributeListEditorWidget::currentCellChanged);
    connect(mTable, &QTableWidget::cellChanged,
            this, &AttributeListEditorWidget::tableCellChanged);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mTable);

    updateTable();
}

AttributeListEditorWidget::~AttributeListEditorWidget() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void AttributeListEditorWidget::setAttributeList(const AttributeList& list) noexcept
{
    mAttributeList = list;
    mSelectedAttribute = nullptr;
    updateTable();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void AttributeListEditorWidget::currentCellChanged(int currentRow, int currentColumn,
                                                   int previousRow, int previousColumn) noexcept
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    mSelectedAttribute = mAttributeList.value(rowToIndex(currentRow)).get();
}

void AttributeListEditorWidget::tableCellChanged(int row, int column) noexcept
{
    QTableWidgetItem* item = mTable->item(row, column); Q_ASSERT(item);

    if (isNewAttributeRow(row)) {
        if (column == COLUMN_KEY) {
            item->setText(cleanKey(item->text()));
        }
    } else if (isExistingAttributeRow(row)) {
        if (column == COLUMN_KEY) {
            item->setText(*setKey(rowToIndex(row), cleanKey(item->text())));
        } else if (column == COLUMN_VALUE) {
            item->setText(setValue(rowToIndex(row), item->text().trimmed()));
        }
    }
}

void AttributeListEditorWidget::attributeTypeChanged(const AttributeType* type) noexcept
{
    Q_ASSERT(type);
    int row = getRowOfTableCellWidget(sender());
    if (isNewAttributeRow(row)) {
        // clear value if it is no longer valid
        QTableWidgetItem* valueItem = mTable->item(row, COLUMN_VALUE); Q_ASSERT(valueItem);
        if (!type->isValueValid(valueItem->text())) {
            valueItem->setText(QString());
        }
        // update unit
        AttributeUnitComboBox* unitComboBox = dynamic_cast<AttributeUnitComboBox*>
            (mTable->cellWidget(row, COLUMN_UNIT)); Q_ASSERT(unitComboBox);
        unitComboBox->setAttributeType(*type);
    } else if (isExistingAttributeRow(row)) {
        setType(rowToIndex(row), *type);
    }
}

void AttributeListEditorWidget::attributeUnitChanged(const AttributeUnit* unit) noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (isExistingAttributeRow(row)) {
        setUnit(rowToIndex(row), unit);
    }
}

void AttributeListEditorWidget::btnAddRemoveClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (isNewAttributeRow(row)) {
        QString key, value;
        const AttributeType* type;
        const AttributeUnit* unit;
        getTableRowContent(row, key, type, value, unit);
        addAttribute(cleanKey(key), *type, value.trimmed(), unit);
    } else if (isExistingAttributeRow(row)) {
        removeAttribute(rowToIndex(row));
    }
}

void AttributeListEditorWidget::btnUpClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (!isExistingAttributeRow(row)) return;
    int index = rowToIndex(row);
    if (index <= 0) return;
    moveAttributeUp(index);
}

void AttributeListEditorWidget::btnDownClicked() noexcept
{
    int row = getRowOfTableCellWidget(sender());
    if (!isExistingAttributeRow(row)) return;
    int index = rowToIndex(row);
    if (index >= mAttributeList.count() - 1) return;
    moveAttributeDown(index);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void AttributeListEditorWidget::updateTable(const Attribute* selected) noexcept
{
    blockSignals(true);

    // remove all rows
    int selectedRow = newAttributeRow();
    mTable->clearSelection();
    mTable->clearContents();
    mTable->setRowCount(mAttributeList.count() + 1);

    // special row for adding a new attribute
    setTableRowContent(newAttributeRow(), "", AttrTypeString::instance(), "", nullptr);

    // existing attributes
    for (int i = 0; i < mAttributeList.count(); ++i) {
        std::shared_ptr<const Attribute> attr = mAttributeList.at(i);
        setTableRowContent(indexToRow(i), *attr->getKey(), attr->getType(),
                           attr->getValueTr(false), attr->getUnit());
        if (attr.get() == selected) {
            selectedRow = indexToRow(i);
        }
    }

    // workaround to trigger column resizing because sometimes auto-resizing does not work
    mTable->hide(); mTable->show();

    // set selected row
    mTable->selectRow(selectedRow);
    mSelectedAttribute = selected;

    blockSignals(false);
}

void AttributeListEditorWidget::setTableRowContent(int row, const QString& key,
    const AttributeType& type, const QString& value, const AttributeUnit* unit) noexcept
{
    // key
    mTable->setItem(row, COLUMN_KEY, new QTableWidgetItem(key));

    // type
    AttributeTypeComboBox* typeComboBox = new AttributeTypeComboBox(this);
    typeComboBox->setProperty("row", row);
    typeComboBox->setStyleSheet("padding: 0px 3px 0px 3px;"); // reduce required space
    typeComboBox->setCurrentItem(type);
    connect(typeComboBox, &AttributeTypeComboBox::currentItemChanged,
            this, &AttributeListEditorWidget::attributeTypeChanged);
    mTable->setCellWidget(row, COLUMN_TYPE, typeComboBox);

    // value
    mTable->setItem(row, COLUMN_VALUE, new QTableWidgetItem(value));

    // unit
    AttributeUnitComboBox* unitComboBox = new AttributeUnitComboBox(this);
    unitComboBox->setProperty("row", row);
    unitComboBox->setStyleSheet("padding: 0px 3px 0px 3px;"); // reduce required space
    unitComboBox->setAttributeType(type);
    unitComboBox->setCurrentItem(unit);
    connect(unitComboBox, &AttributeUnitComboBox::currentItemChanged,
            this, &AttributeListEditorWidget::attributeUnitChanged);
    mTable->setCellWidget(row, COLUMN_UNIT, unitComboBox);

    // buttons
    int btnSize = typeComboBox->sizeHint().height();
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
            this, &AttributeListEditorWidget::btnAddRemoveClicked);
    if (isExistingAttributeRow(row)) {
        btnAddRemove->setFixedWidth(btnSize);
        btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));
        QToolButton* btnUp = new QToolButton(buttonsColumnWidget);
        btnUp->setProperty("row", row);
        btnUp->setFixedSize(btnSize, btnSize);
        btnUp->setIcon(QIcon(":/img/actions/up.png"));
        btnUp->setIconSize(iconSize);
        btnUp->setEnabled(rowToIndex(row) > 0);
        connect(btnUp, &QToolButton::clicked,
                this, &AttributeListEditorWidget::btnUpClicked);
        buttonsColumnLayout->addWidget(btnUp);
        QToolButton* btnDown = new QToolButton(buttonsColumnWidget);
        btnDown->setProperty("row", row);
        btnDown->setFixedSize(btnSize, btnSize);
        btnDown->setIcon(QIcon(":/img/actions/down.png"));
        btnDown->setIconSize(iconSize);
        btnDown->setEnabled(rowToIndex(row) < mAttributeList.count() - 1);
        connect(btnDown, &QToolButton::clicked,
                this, &AttributeListEditorWidget::btnDownClicked);
        buttonsColumnLayout->addWidget(btnDown);
    } else {
        btnAddRemove->setFixedWidth(btnSize * 3);
        btnAddRemove->setIcon(QIcon(":/img/actions/add.png"));
    }
    buttonsColumnLayout->addWidget(btnAddRemove);
    mTable->setCellWidget(row, COLUMN_BUTTONS, buttonsColumnWidget);

    // adjust the height of the row according to the size of the contained widgets
    mTable->verticalHeader()->resizeSection(row, btnSize);
}

void AttributeListEditorWidget::getTableRowContent(int row, QString& key,
    const AttributeType*& type, QString& value, const AttributeUnit*& unit) const noexcept
{
    // key
    const QTableWidgetItem* keyItem = mTable->item(row, COLUMN_KEY); Q_ASSERT(keyItem);
    key = keyItem->text();

    // type
    const AttributeTypeComboBox* typeComboBox = dynamic_cast<const AttributeTypeComboBox*>
        (mTable->cellWidget(row, COLUMN_TYPE)); Q_ASSERT(typeComboBox);
    type = &typeComboBox->getCurrentItem();

    // value
    const QTableWidgetItem* valueItem = mTable->item(row, COLUMN_VALUE); Q_ASSERT(valueItem);
    value = valueItem->text();

    // unit
    const AttributeUnitComboBox* unitComboBox = dynamic_cast<const AttributeUnitComboBox*>
        (mTable->cellWidget(row, COLUMN_UNIT)); Q_ASSERT(unitComboBox);
    unit = unitComboBox->getCurrentItem();
}

void AttributeListEditorWidget::addAttribute(const QString& key, const AttributeType& type,
                                             const QString& value, const AttributeUnit* unit) noexcept
{
    try {
        AttributeKey attrKey = convertStringToKeyOrThrow(key); // can throw
        throwIfValueInvalid(type, value);
        mAttributeList.append(std::make_shared<Attribute>(attrKey, type, value, unit)); // can throw
        updateTable();
        emit edited(mAttributeList);
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not add attribute"), e.getMsg());
    }
}

void AttributeListEditorWidget::removeAttribute(int index) noexcept
{
    Q_ASSERT(index >= 0 && index < mAttributeList.count());
    mAttributeList.remove(index);
    updateTable(mSelectedAttribute);
    emit edited(mAttributeList);
}

void AttributeListEditorWidget::moveAttributeUp(int index) noexcept
{
    Q_ASSERT(index >= 1 && index < mAttributeList.count());
    mAttributeList.swap(index, index - 1);
    updateTable(mSelectedAttribute);
    emit edited(mAttributeList);
}

void AttributeListEditorWidget::moveAttributeDown(int index) noexcept
{
    Q_ASSERT(index >= 0 && index < mAttributeList.count() - 1);
    mAttributeList.swap(index, index + 1);
    updateTable(mSelectedAttribute);
    emit edited(mAttributeList);
}

AttributeKey AttributeListEditorWidget::setKey(int index, const QString& key) noexcept
{
    Attribute& attr = *mAttributeList[index];
    if (attr.getKey() == key) {
        return attr.getKey();
    }

    try {
        AttributeKey attrKey = convertStringToKeyOrThrow(key); // can throw
        attr.setKey(attrKey);
        emit edited(mAttributeList);
        return attrKey;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Invalid key"), e.getMsg());
        return attr.getKey();
    }
}

void AttributeListEditorWidget::setType(int index, const AttributeType& type) noexcept
{
    try {
        Attribute& attr = *mAttributeList[index];

        // clear value if it is no longer valid
        QString value = attr.getValue();
        if (!type.isValueValid(value)) {
            value = QString();
        }

        // reset unit if it is no longer valid
        const AttributeUnit* unit = attr.getUnit();
        if (!type.isUnitAvailable(unit)) {
            unit = type.getDefaultUnit();
        }

        // apply values
        attr.setTypeValueUnit(type, value, unit);

        // update table
        mTable->setItem(indexToRow(index), COLUMN_VALUE, new QTableWidgetItem(value));
        AttributeUnitComboBox* unitComboBox = dynamic_cast<AttributeUnitComboBox*>
            (mTable->cellWidget(indexToRow(index), COLUMN_UNIT)); Q_ASSERT(unitComboBox);
        unitComboBox->setAttributeType(type);
        unitComboBox->setCurrentItem(unit);

        emit edited(mAttributeList);
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
}

QString AttributeListEditorWidget::setValue(int index, const QString& value) noexcept
{
    Attribute& attr = *mAttributeList[index];
    if (attr.getValue() == value) {
        return attr.getValue();
    }

    try {
        throwIfValueInvalid(attr.getType(), value);
        attr.setTypeValueUnit(attr.getType(), value, attr.getUnit());
        emit edited(mAttributeList);
        return value;
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
        return attr.getValue();
    }
}

void AttributeListEditorWidget::setUnit(int index, const AttributeUnit* unit) noexcept
{
    try {
        Attribute& attr = *mAttributeList[index];
        attr.setTypeValueUnit(attr.getType(), attr.getValue(), unit);
        emit edited(mAttributeList);
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
}

int AttributeListEditorWidget::getRowOfTableCellWidget(QObject* obj) const noexcept
{
    bool ok = false;
    int row = obj->property("row").toInt(&ok); Q_ASSERT(ok);
    Q_ASSERT(row >= 0 && row < mTable->rowCount());
    return row;
}

AttributeKey AttributeListEditorWidget::convertStringToKeyOrThrow(const QString& key) const
{
    if (key.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The key must not be empty.")).arg(key));
    }
    if (mAttributeList.contains(key)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already an attribute with the key \"%1\".")).arg(key));
    }
    return AttributeKey(key); // can throw
}

void AttributeListEditorWidget::throwIfValueInvalid(const AttributeType& type,
                                                    const QString& value) const
{
    if (!type.isValueValid(value)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The value \"%1\" is invalid.")).arg(value));
    }
}

QString AttributeListEditorWidget::cleanKey(const QString& key) noexcept
{
    // TODO: it's ugly to use a method from FilePath...
    QString str = FilePath::cleanFileName(key, FilePath::ReplaceSpaces | FilePath::ToUpperCase);
    return str.replace(QRegularExpression("[^_0-9A-Z]"), "_");
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
