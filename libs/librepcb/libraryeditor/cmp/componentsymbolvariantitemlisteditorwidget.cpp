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
#include "componentsymbolvariantitemlisteditorwidget.h"

#include "../common/symbolchooserdialog.h"

#include <librepcb/common/widgets/centeredcheckbox.h>
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

ComponentSymbolVariantItemListEditorWidget::
    ComponentSymbolVariantItemListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mTable(new QTableWidget(this)),
    mWorkspace(nullptr),
    mLayerProvider(nullptr),
    mItems(nullptr),
    mNewSymbolLabel(nullptr),
    mNewRequiredCheckbox(nullptr) {
  mTable->setCornerButtonEnabled(false);
  mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTable->setSelectionMode(QAbstractItemView::SingleSelection);
  mTable->setColumnCount(_COLUMN_COUNT);
  mTable->setHorizontalHeaderItem(COLUMN_NUMBER, new QTableWidgetItem(tr("#")));
  mTable->setHorizontalHeaderItem(COLUMN_SYMBOL,
                                  new QTableWidgetItem(tr("Symbol")));
  mTable->setHorizontalHeaderItem(COLUMN_SUFFIX,
                                  new QTableWidgetItem(tr("Suffix")));
  mTable->setHorizontalHeaderItem(COLUMN_ISREQUIRED,
                                  new QTableWidgetItem(tr("Required")));
  mTable->setHorizontalHeaderItem(COLUMN_POS_X,
                                  new QTableWidgetItem(tr("Position X")));
  mTable->setHorizontalHeaderItem(COLUMN_POS_Y,
                                  new QTableWidgetItem(tr("Position Y")));
  mTable->setHorizontalHeaderItem(COLUMN_ROTATION,
                                  new QTableWidgetItem(tr("Rotation")));
  mTable->setHorizontalHeaderItem(COLUMN_BUTTONS,
                                  new QTableWidgetItem(tr("Actions")));
  mTable->horizontalHeaderItem(COLUMN_NUMBER)
      ->setTextAlignment(Qt::AlignCenter);
  mTable->horizontalHeaderItem(COLUMN_SUFFIX)
      ->setTextAlignment(Qt::AlignCenter);
  mTable->horizontalHeaderItem(COLUMN_ISREQUIRED)
      ->setTextAlignment(Qt::AlignCenter);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_NUMBER, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(COLUMN_SYMBOL,
                                                   QHeaderView::Stretch);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_SUFFIX, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_ISREQUIRED, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_POS_X, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_POS_Y, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_ROTATION, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setSectionResizeMode(
      COLUMN_BUTTONS, QHeaderView::ResizeToContents);
  mTable->horizontalHeader()->setMinimumSectionSize(10);
  mTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  mTable->verticalHeader()->setMinimumSectionSize(20);
  connect(mTable, &QTableWidget::currentCellChanged, this,
          &ComponentSymbolVariantItemListEditorWidget::currentCellChanged);
  connect(mTable, &QTableWidget::cellChanged, this,
          &ComponentSymbolVariantItemListEditorWidget::tableCellChanged);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mTable);
}

ComponentSymbolVariantItemListEditorWidget::
    ~ComponentSymbolVariantItemListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSymbolVariantItemListEditorWidget::setVariant(
    const workspace::Workspace&     ws,
    const IF_GraphicsLayerProvider& layerProvider,
    ComponentSymbolVariantItemList& items) noexcept {
  mWorkspace     = &ws;
  mLayerProvider = &layerProvider;
  mItems         = &items;
  mSelectedItem  = tl::nullopt;
  updateTable();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void ComponentSymbolVariantItemListEditorWidget::currentCellChanged(
    int currentRow, int currentColumn, int previousRow,
    int previousColumn) noexcept {
  Q_UNUSED(currentColumn);
  Q_UNUSED(previousRow);
  Q_UNUSED(previousColumn);
  mSelectedItem = getUuidOfRow(currentRow);
}

void ComponentSymbolVariantItemListEditorWidget::tableCellChanged(
    int row, int column) noexcept {
  QTableWidgetItem* item = mTable->item(row, column);
  Q_ASSERT(item);
  tl::optional<Uuid> uuid = getUuidOfRow(row);

  switch (column) {
    case COLUMN_SUFFIX: {
      item->setText(item->text().trimmed().toUpper());
      if (isExistingItemRow(row) && uuid) {
        setSuffix(*uuid, item->text());
      }
      break;
    }
    case COLUMN_POS_X: {
      if (isExistingItemRow(row) && uuid) {
        try {
          setPosX(*uuid, Length::fromMm(item->text().trimmed()));
        } catch (const Exception& e) {
          QMessageBox::warning(this, tr("Error"), e.getMsg());
        }
      }
      break;
    }
    case COLUMN_POS_Y: {
      if (isExistingItemRow(row) && uuid) {
        try {
          setPosY(*uuid, Length::fromMm(item->text().trimmed()));
        } catch (const Exception& e) {
          QMessageBox::warning(this, tr("Error"), e.getMsg());
        }
      }
      break;
    }
    case COLUMN_ROTATION: {
      if (isExistingItemRow(row) && uuid) {
        try {
          setRotation(*uuid, Angle::fromDeg(item->text().trimmed()));
        } catch (const Exception& e) {
          QMessageBox::warning(this, tr("Error"), e.getMsg());
        }
      }
      break;
    }
    default: { break; }
  }
}

void ComponentSymbolVariantItemListEditorWidget::isRequiredChanged(
    bool checked) noexcept {
  int                row  = getRowOfTableCellWidget(sender());
  tl::optional<Uuid> uuid = getUuidOfRow(row);
  if (isExistingItemRow(row) && uuid) {
    setIsRequired(*uuid, checked);
  }
}

void ComponentSymbolVariantItemListEditorWidget::
    btnChooseSymbolClicked() noexcept {
  SymbolChooserDialog dialog(*mWorkspace, *mLayerProvider, this);
  if ((dialog.exec() == QDialog::Accepted) && dialog.getSelectedSymbolUuid()) {
    int                row  = getRowOfTableCellWidget(sender());
    tl::optional<Uuid> uuid = getUuidOfRow(row);
    if (isNewItemRow(row)) {
      mNewSymbolLabel->setText(dialog.getSelectedSymbolNameTr());
      mNewSymbolLabel->setToolTip(dialog.getSelectedSymbolUuid()->toStr());
    } else if (isExistingItemRow(row) && uuid) {
      setSymbolUuid(*uuid, *dialog.getSelectedSymbolUuid());
    }
  }
}

void ComponentSymbolVariantItemListEditorWidget::
    btnAddRemoveClicked() noexcept {
  int                row  = getRowOfTableCellWidget(sender());
  tl::optional<Uuid> uuid = getUuidOfRow(row);
  if (isNewItemRow(row)) {
    try {
      const QTableWidgetItem* suffixItem = mTable->item(row, COLUMN_SUFFIX);
      Q_ASSERT(suffixItem);
      const QTableWidgetItem* posXItem = mTable->item(row, COLUMN_POS_X);
      Q_ASSERT(posXItem);
      const QTableWidgetItem* posYItem = mTable->item(row, COLUMN_POS_Y);
      Q_ASSERT(posYItem);
      const QTableWidgetItem* rotItem = mTable->item(row, COLUMN_ROTATION);
      Q_ASSERT(rotItem);
      Uuid symbolUuid =
          Uuid::fromString(mNewSymbolLabel->toolTip());  // can throw
      Point pos(Length::fromMm(posXItem->text()),
                Length::fromMm(posYItem->text()));
      Angle rot(Angle::fromDeg(rotItem->text()));
      addItem(symbolUuid, suffixItem->text().trimmed(),
              mNewRequiredCheckbox->isChecked(), pos, rot);
    } catch (const Exception& e) {
      QMessageBox::warning(this, tr("Error"), e.getMsg());
    }
  } else if (isExistingItemRow(row) && uuid) {
    removeItem(*uuid);
  }
}

void ComponentSymbolVariantItemListEditorWidget::btnUpClicked() noexcept {
  int row = getRowOfTableCellWidget(sender());
  if (!isExistingItemRow(row)) return;
  int index = rowToIndex(row);
  if (index <= 0) return;
  moveItemUp(index);
}

void ComponentSymbolVariantItemListEditorWidget::btnDownClicked() noexcept {
  int row = getRowOfTableCellWidget(sender());
  if (!isExistingItemRow(row)) return;
  int index = rowToIndex(row);
  if (index >= mItems->count() - 1) return;
  moveItemDown(index);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSymbolVariantItemListEditorWidget::updateTable(
    tl::optional<Uuid> selected) noexcept {
  blockSignals(true);

  // memorize content of new item row
  tl::optional<Uuid> newSymbolUuid = Uuid::tryFromString(
      mNewSymbolLabel ? mNewSymbolLabel->toolTip() : QString());
  bool newRequired =
      mNewRequiredCheckbox ? mNewRequiredCheckbox->isChecked() : true;
  Point newPosition;
  Angle newRotation;

  // clear table
  mNewSymbolLabel      = nullptr;
  mNewRequiredCheckbox = nullptr;
  int selectedRow      = newItemRow();
  mTable->clearSelection();
  mTable->clearContents();
  mTable->setRowCount(mItems->count() + 1);

  // special row for adding a new item
  setTableRowContent(newItemRow(), mItems->count() + 1, tl::nullopt,
                     newSymbolUuid, "", newRequired, newPosition, newRotation);

  // existing signals
  for (int i = 0; i < mItems->count(); ++i) {
    const ComponentSymbolVariantItem& item = *mItems->at(i);
    setTableRowContent(indexToRow(i), i + 1, item.getUuid(),
                       item.getSymbolUuid(), *item.getSuffix(),
                       item.isRequired(), item.getSymbolPosition(),
                       item.getSymbolRotation());
    if (item.getUuid() == selected) {
      selectedRow = indexToRow(i);
    }
  }

  // workaround to trigger column resizing because sometimes auto-resizing does
  // not work
  mTable->hide();
  mTable->show();

  // set selected row
  mTable->selectRow(selectedRow);
  mSelectedItem = selected;

  blockSignals(false);
}

void ComponentSymbolVariantItemListEditorWidget::setTableRowContent(
    int row, int number, const tl::optional<Uuid>& uuid,
    const tl::optional<Uuid>& symbol, const QString& suffix, bool required,
    const Point& pos, const Angle& rot) noexcept {
  // header
  QString header =
      uuid ? uuid->toStr().left(13) % "..." : tr("Add new symbol:");
  QTableWidgetItem* headerItem = new QTableWidgetItem(header);
  headerItem->setToolTip(uuid ? uuid->toStr() : QString());
  QFont headerFont = headerItem->font();
  headerFont.setStyleHint(
      QFont::Monospace);  // ensure that the column width is fixed
  headerFont.setFamily("Monospace");
  headerItem->setFont(headerFont);
  mTable->setVerticalHeaderItem(row, headerItem);

  // number
  QTableWidgetItem* numberItem = new QTableWidgetItem(QString::number(number));
  numberItem->setFlags(numberItem->flags() &
                       ~Qt::ItemFlags(Qt::ItemIsEditable));
  numberItem->setTextAlignment(Qt::AlignCenter);
  mTable->setItem(row, COLUMN_NUMBER, numberItem);

  // symbol
  int      btnSize = 23;  // TODO: can we determine this value dynamically?
  QWidget* symbolColumnWidget = new QWidget(this);
  symbolColumnWidget->setSizePolicy(QSizePolicy::MinimumExpanding,
                                    QSizePolicy::Fixed);
  QHBoxLayout* symbolColumnLayout = new QHBoxLayout(symbolColumnWidget);
  symbolColumnLayout->setContentsMargins(0, 0, 0, 0);
  symbolColumnLayout->setSpacing(0);
  QLabel* symbolLabel = new QLabel(this);
  symbolLabel->setIndent(5);
  if (symbol) {
    try {
      QString           symName;
      const QStringList lo =
          mWorkspace->getSettings().getLibLocaleOrder().getLocaleOrder();
      FilePath symFp =
          mWorkspace->getLibraryDb().getLatestSymbol(*symbol);  // can throw
      mWorkspace->getLibraryDb().getElementTranslations<Symbol>(
          symFp, lo, &symName);  // can throw
      symbolLabel->setText(symName);
      symbolLabel->setToolTip(symbol->toStr());
    } catch (const Exception& e) {
      symbolLabel->setText(symbol->toStr());
      symbolLabel->setToolTip(e.getMsg());
      symbolLabel->setStyleSheet("color: red;");
    }
  }
  symbolColumnLayout->addWidget(symbolLabel);
  QToolButton* symbolButton = new QToolButton(this);
  symbolButton->setProperty("row", row);
  symbolButton->setFixedSize(btnSize, btnSize);
  symbolButton->setText("...");
  connect(symbolButton, &QToolButton::clicked, this,
          &ComponentSymbolVariantItemListEditorWidget::btnChooseSymbolClicked);
  symbolColumnLayout->addWidget(symbolButton);
  mTable->setCellWidget(row, COLUMN_SYMBOL, symbolColumnWidget);
  if (isNewItemRow(row)) mNewSymbolLabel = symbolLabel;

  // suffix
  QTableWidgetItem* suffixItem = new QTableWidgetItem(suffix);
  suffixItem->setTextAlignment(Qt::AlignCenter);
  mTable->setItem(row, COLUMN_SUFFIX, suffixItem);

  // required
  CenteredCheckBox* requiredCheckBox = new CenteredCheckBox(this);
  requiredCheckBox->setProperty("row", row);
  requiredCheckBox->setChecked(required);
  connect(requiredCheckBox, &CenteredCheckBox::toggled, this,
          &ComponentSymbolVariantItemListEditorWidget::isRequiredChanged);
  mTable->setCellWidget(row, COLUMN_ISREQUIRED, requiredCheckBox);
  if (isNewItemRow(row)) mNewRequiredCheckbox = requiredCheckBox;

  // position x
  QTableWidgetItem* posXItem = new QTableWidgetItem(pos.getX().toMmString());
  // posXItem->setTextAlignment(Qt::AlignCenter);
  mTable->setItem(row, COLUMN_POS_X, posXItem);

  // position y
  QTableWidgetItem* posYItem = new QTableWidgetItem(pos.getY().toMmString());
  // posYItem->setTextAlignment(Qt::AlignCenter);
  mTable->setItem(row, COLUMN_POS_Y, posYItem);

  // rotation
  QTableWidgetItem* rotItem = new QTableWidgetItem(rot.toDegString());
  // rotItem->setTextAlignment(Qt::AlignCenter);
  mTable->setItem(row, COLUMN_ROTATION, rotItem);

  // buttons
  QSize    iconSize(btnSize - 6, btnSize - 6);
  QWidget* buttonsColumnWidget = new QWidget(this);
  buttonsColumnWidget->setSizePolicy(QSizePolicy::MinimumExpanding,
                                     QSizePolicy::Fixed);
  QHBoxLayout* buttonsColumnLayout = new QHBoxLayout(buttonsColumnWidget);
  buttonsColumnLayout->setContentsMargins(0, 0, 0, 0);
  buttonsColumnLayout->setSpacing(0);
  QToolButton* btnAddRemove = new QToolButton(buttonsColumnWidget);
  btnAddRemove->setProperty("row", row);
  btnAddRemove->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::Fixed);
  btnAddRemove->setFixedHeight(btnSize);
  btnAddRemove->setIconSize(iconSize);
  connect(btnAddRemove, &QToolButton::clicked, this,
          &ComponentSymbolVariantItemListEditorWidget::btnAddRemoveClicked);
  if (isExistingItemRow(row)) {
    btnAddRemove->setFixedWidth(btnSize);
    btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));

    QToolButton* btnUp = new QToolButton(buttonsColumnWidget);
    btnUp->setProperty("row", row);
    btnUp->setFixedSize(btnSize, btnSize);
    btnUp->setIcon(QIcon(":/img/actions/up.png"));
    btnUp->setIconSize(iconSize);
    btnUp->setEnabled(rowToIndex(row) > 0);
    connect(btnUp, &QToolButton::clicked, this,
            &ComponentSymbolVariantItemListEditorWidget::btnUpClicked);
    buttonsColumnLayout->addWidget(btnUp);

    QToolButton* btnDown = new QToolButton(buttonsColumnWidget);
    btnDown->setProperty("row", row);
    btnDown->setFixedSize(btnSize, btnSize);
    btnDown->setIcon(QIcon(":/img/actions/down.png"));
    btnDown->setIconSize(iconSize);
    btnDown->setEnabled(rowToIndex(row) < mItems->count() - 1);
    connect(btnDown, &QToolButton::clicked, this,
            &ComponentSymbolVariantItemListEditorWidget::btnDownClicked);
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

void ComponentSymbolVariantItemListEditorWidget::addItem(
    const Uuid& symbol, const QString& suffix, bool required, const Point& pos,
    const Angle& rot) noexcept {
  try {
    ComponentSymbolVariantItemSuffix constrainedSuffix(suffix);  // can throw
    FilePath                         fp =
        mWorkspace->getLibraryDb().getLatestSymbol(symbol);     // can throw
    Symbol                                      sym(fp, true);  // can throw
    std::shared_ptr<ComponentSymbolVariantItem> item(
        new ComponentSymbolVariantItem(Uuid::createRandom(), symbol, pos, rot,
                                       required, constrainedSuffix));
    item->getPinSignalMap() =
        ComponentPinSignalMapHelpers::create(sym.getPins().getUuidSet());
    mItems->append(item);
    updateTable();
    emit edited();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not add symbol"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListEditorWidget::removeItem(
    const Uuid& uuid) noexcept {
  Q_ASSERT(mItems->contains(uuid));
  mItems->remove(uuid);
  updateTable(mSelectedItem);
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::moveItemUp(
    int index) noexcept {
  Q_ASSERT(index >= 1 && index < mItems->count());
  mItems->swap(index, index - 1);
  updateTable(mSelectedItem);
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::moveItemDown(
    int index) noexcept {
  Q_ASSERT(index >= 0 && index < mItems->count() - 1);
  mItems->swap(index, index + 1);
  updateTable(mSelectedItem);
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::setSymbolUuid(
    const Uuid& uuid, const Uuid& symbol) noexcept {
  Q_ASSERT(mItems->contains(uuid));
  try {
    FilePath fp =
        mWorkspace->getLibraryDb().getLatestSymbol(symbol);  // can throw
    Symbol sym(fp, true);                                    // can throw
    mItems->find(uuid)->setSymbolUuid(symbol);
    mItems->find(uuid)->getPinSignalMap() =
        ComponentPinSignalMapHelpers::create(sym.getPins().getUuidSet());
    updateTable(mSelectedItem);
    emit edited();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not change symbol"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListEditorWidget::setIsRequired(
    const Uuid& uuid, bool required) noexcept {
  Q_ASSERT(mItems->contains(uuid));
  mItems->find(uuid)->setIsRequired(required);
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::setSuffix(
    const Uuid& uuid, const QString& suffix) noexcept {
  try {
    Q_ASSERT(mItems->contains(uuid));
    mItems->find(uuid)->setSuffix(
        ComponentSymbolVariantItemSuffix(suffix));  // can throw
    emit edited();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void ComponentSymbolVariantItemListEditorWidget::setPosX(
    const Uuid& uuid, const Length& x) noexcept {
  Q_ASSERT(mItems->contains(uuid));
  Point pos = mItems->find(uuid)->getSymbolPosition();
  pos.setX(x);
  mItems->find(uuid)->setSymbolPosition(pos);
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::setPosY(
    const Uuid& uuid, const Length& y) noexcept {
  Q_ASSERT(mItems->contains(uuid));
  Point pos = mItems->find(uuid)->getSymbolPosition();
  pos.setY(y);
  mItems->find(uuid)->setSymbolPosition(pos);
  emit edited();
}

void ComponentSymbolVariantItemListEditorWidget::setRotation(
    const Uuid& uuid, const Angle& rot) noexcept {
  Q_ASSERT(mItems->contains(uuid));
  mItems->find(uuid)->setSymbolRotation(rot);
  emit edited();
}

int ComponentSymbolVariantItemListEditorWidget::getRowOfTableCellWidget(
    QObject* obj) const noexcept {
  bool ok  = false;
  int  row = obj->property("row").toInt(&ok);
  Q_ASSERT(ok);
  Q_ASSERT(row >= 0 && row < mTable->rowCount());
  return row;
}

tl::optional<Uuid> ComponentSymbolVariantItemListEditorWidget::getUuidOfRow(
    int row) const noexcept {
  if (isExistingItemRow(row)) {
    return mItems->value(rowToIndex(row))->getUuid();
  } else {
    return tl::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
