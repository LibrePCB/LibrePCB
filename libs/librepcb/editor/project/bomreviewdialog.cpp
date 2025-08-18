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
#include "bomreviewdialog.h"

#include "../modelview/partinformationdelegate.h"
#include "../workspace/desktopservices.h"
#include "partinformationtooltip.h"
#include "ui_bomreviewdialog.h"

#include <librepcb/core/export/bom.h>
#include <librepcb/core/export/bomcsvwriter.h>
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/bomgenerator.h>
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BomReviewDialog::BomReviewDialog(const WorkspaceSettings& settings,
                                 Project& project, const Board* board,
                                 QWidget* parent) noexcept
  : QDialog(parent),
    mSettings(settings),
    mProject(project),
    mBom(new Bom(QStringList(), {})),
    mUi(new Ui::BomReviewDialog),
    mPartToolTip(new PartInformationToolTip(settings, this)),
    mPartInfoProgress(0),
    mUpdatePartInformationScheduled(false) {
  mUi->setupUi(this);
  mUi->tableWidget->setWordWrap(false);
  // Note: Don't stretch columns since it leads to cropped text in some
  // columns and unused space in other columns. Better resize all columns
  // to their content and show a horizontal scrollbar when needed.
  mUi->tableWidget->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Interactive);
  mUi->tableWidget->verticalHeader()->setMinimumSectionSize(10);
  mUi->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mUi->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  const bool multipleAssemblyVariants =
      mProject.getCircuit().getAssemblyVariants().count() > 1;
  mUi->lblAssemblyVariant->setVisible(multipleAssemblyVariants);
  mUi->cbxAssemblyVariant->setVisible(multipleAssemblyVariants);
  mUi->lblNote->setText("ⓘ " % mUi->lblNote->text());

  // Setup part information tooltip.
  auto setProviderInfo = [this]() {
    mPartToolTip->setProviderInfo(
        PartInformationProvider::instance().getProviderName(),
        PartInformationProvider::instance().getProviderUrl(),
        PartInformationProvider::instance().getProviderLogo(),
        PartInformationProvider::instance().getInfoUrl());
  };
  setProviderInfo();
  connect(&PartInformationProvider::instance(),
          &PartInformationProvider::providerInfoChanged, this, setProviderInfo);
  mUi->tableWidget->setMouseTracking(true);
  mUi->tableWidget->installEventFilter(this);
  mPartToolTip->installEventFilter(this);
  connect(
      mUi->tableWidget, &QTableWidget::itemEntered, this,
      [this](QTableWidgetItem* item) {
        if (item) {
          const auto data =
              item->data(Qt::UserRole).value<PartInformationDelegate::Data>();
          if (data.info && (data.info->results == 1)) {
            const QRect rect =
                mUi->tableWidget->visualItemRect(item).intersected(
                    mUi->tableWidget->viewport()->rect());
            const QPoint pos = mUi->tableWidget->viewport()->mapToGlobal(
                QPoint(rect.right(), rect.center().y()));
            mPartToolTip->showPart(data.info, pos);
          } else {
            mPartToolTip->hideAndReset(!data.initialized);
          }
        } else {
          mPartToolTip->hideAndReset();
        }
      });

  // List boards.
  mUi->cbxBoard->addItem(tr("None"));
  foreach (const Board* brd, mProject.getBoards()) {
    mUi->cbxBoard->addItem(*brd->getName());
  }

  // List assembly variants.
  for (const auto& av : mProject.getCircuit().getAssemblyVariants()) {
    mUi->cbxAssemblyVariant->addItem(av.getDisplayText(), av.getUuid().toStr());
  }
  mUi->cbxAssemblyVariant->setCurrentIndex(0);
  mUi->cbxAssemblyVariant->setEnabled(mUi->cbxAssemblyVariant->count() > 1);

  // List attributes.
  mUi->edtAttributes->setText(mProject.getCustomBomAttributes().join(", "));

  // Select board.
  int index = board ? mProject.getBoardIndex(*board) + 1 : 0;
  mUi->cbxBoard->setCurrentIndex(index);
  updateBom();

  // Setup automatic update of parts information.
  QTimer* partInfoTimer = new QTimer(this);
  partInfoTimer->setInterval(250);
  connect(partInfoTimer, &QTimer::timeout, this, [this]() {
    ++mPartInfoProgress;
    if (mUpdatePartInformationScheduled) {
      updatePartsInformation();
    }
  });
  partInfoTimer->start();
  connect(&PartInformationProvider::instance(),
          &PartInformationProvider::serviceOperational, this,
          &BomReviewDialog::updatePartsInformation);
  connect(&PartInformationProvider::instance(),
          &PartInformationProvider::newPartsInformationAvailable, this,
          &BomReviewDialog::updatePartsInformation);

  connect(
      mUi->cbxBoard,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BomReviewDialog::updateBom);
  connect(
      mUi->cbxAssemblyVariant,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BomReviewDialog::updateBom);
  connect(mUi->edtAttributes, &QLineEdit::textEdited, this,
          &BomReviewDialog::updateAttributes);
  connect(mUi->tableWidget, &QTableWidget::cellDoubleClicked, this,
          &BomReviewDialog::tableCellDoubleClicked);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &BomReviewDialog::reject);

  // Load the window geometry and settings.
  // Note: Do not use restoreGeometry(), only store the window size (but not
  // the position) since the dialog shall be centered within the parent window.
  QSettings cs;
  const QSize size = cs.value("bom_generator_dialog/window_size").toSize();
  if (size.isValid()) {
    resize(size);
  }
}

BomReviewDialog::~BomReviewDialog() noexcept {
  // Save the window geometry and settings.
  QSettings cs;
  cs.setValue("bom_generator_dialog/window_size", size());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BomReviewDialog::eventFilter(QObject* obj, QEvent* e) noexcept {
  if (mPartToolTip && (e->type() == QEvent::Leave) &&
      ((!mPartToolTip->isVisible()) ||
       ((!mPartToolTip->rect().contains(
           mPartToolTip->mapFromGlobal(QCursor::pos())))))) {
    mPartToolTip->hideAndReset();
  }
  return QDialog::eventFilter(obj, e);
}

/*******************************************************************************
 *  GUI Event Handlers
 ******************************************************************************/

void BomReviewDialog::tableCellDoubleClicked(int row, int column) noexcept {
  if (auto item = mUi->tableWidget->item(row, column)) {
    const auto data =
        item->data(Qt::UserRole).value<PartInformationDelegate::Data>();
    if (data.info && data.info->pricingUrl.isValid()) {
      DesktopServices ds(mSettings);
      ds.openWebUrl(data.info->pricingUrl);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BomReviewDialog::updateAttributes() noexcept {
  try {
    QStringList attributes;
    foreach (const QString str,
             mUi->edtAttributes->text().simplified().split(',')) {
      if (!str.trimmed().isEmpty()) {
        attributes.append(str.trimmed());
      }
    }
    if (attributes != mProject.getCustomBomAttributes()) {
      mProject.setCustomBomAttributes(attributes);
      emit projectSettingsModified();
      updateBom();
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to update custom BOM attributes:" << e.getMsg();
  }
}

void BomReviewDialog::updateBom() noexcept {
  if (auto avUuid = getAssemblyVariantUuid(false)) {
    const Board* board =
        mProject.getBoardByIndex(mUi->cbxBoard->currentIndex() - 1);

    BomGenerator gen(mProject);
    gen.setAdditionalAttributes(mProject.getCustomBomAttributes());
    mBom = gen.generate(board, *avUuid);
    updateTable();
  }
}

void BomReviewDialog::updateTable() noexcept {
  mUi->tableWidget->clear();
  mUi->lblTotalPrice->clear();

  try {
    BomCsvWriter writer(*mBom);
    writer.setIncludeNonMountedParts(true);
    std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
    mUi->tableWidget->setRowCount(csv->getValues().count());
    mUi->tableWidget->setColumnCount(csv->getHeader().count());
    mUi->tableWidget->setHorizontalHeaderLabels(
        csv->getHeader() + QStringList{tr("Availability")});
    for (int column = 0; column < csv->getHeader().count(); ++column) {
      for (int row = 0; row < csv->getValues().count(); ++row) {
        QString text = csv->getValues()[row][column];
        text.replace("\n", " ");
        QTableWidgetItem* item = new QTableWidgetItem(text);
        if (csv->getValues()[row][0] == "0") {
          // Don't use hardcoded colors because of light/dark theme support.
          item->setBackground(
              palette().color(QPalette::Disabled, QPalette::Base));
          item->setForeground(palette().color(QPalette::PlaceholderText));
        }
        mUi->tableWidget->setItem(row, column, item);
      }
    }
    foreach (const auto& columnPair, mBom->getMpnManufacturerColumns()) {
      mUi->tableWidget->setItemDelegateForColumn(
          columnPair.second + 2, new PartInformationDelegate(false, this));
    }
    mUi->tableWidget->resizeRowsToContents();
    mUi->tableWidget->resizeColumnsToContents();
    updatePartsInformation();
  } catch (Exception& e) {
    qCritical() << "Failed to update BOM table widget:" << e.getMsg();
  }
}

void BomReviewDialog::updatePartsInformation() noexcept {
  if (!mSettings.autofetchLivePartInformation.get()) {
    return;
  }

  if (!PartInformationProvider::instance().isOperational()) {
    PartInformationProvider::instance().startOperation();
    return;
  }

  mUpdatePartInformationScheduled = false;

  qreal totalPrice = 0;
  int partsWithPrice = 0;
  QSet<int> countedParts;
  foreach (const auto& columnPair, mBom->getMpnManufacturerColumns()) {
    for (int row = 0; row < mBom->getItems().count(); ++row) {
      const BomItem& item = mBom->getItems().at(row);
      QTableWidgetItem* mpnItem =
          mUi->tableWidget->item(row, columnPair.first + 2);
      QTableWidgetItem* manufacturerItem =
          mUi->tableWidget->item(row, columnPair.second + 2);
      if ((!mpnItem) || (!manufacturerItem)) {
        qCritical() << "Invalid MPN/manufacturer cell index in BOM table.";
        continue;
      }
      PartInformationDelegate::Data data =
          manufacturerItem->data(Qt::UserRole)
              .value<PartInformationDelegate::Data>();
      if (!data.initialized) {
        data.part.mpn = mpnItem->text();
        data.part.manufacturer = manufacturerItem->text();
        data.priceQuantity = item.getDesignators().count();
        data.initialized = true;
      }
      if ((!data.info) && (!data.part.mpn.isEmpty()) &&
          (!data.part.manufacturer.isEmpty())) {
        data.info = PartInformationProvider::instance().getPartInfo(data.part);
        if ((!data.info) && (!data.infoRequested)) {
          PartInformationProvider::instance().scheduleRequest(data.part);
          data.infoRequested = true;
        }
        if ((!data.info) && data.infoRequested) {
          if (PartInformationProvider::instance().isOngoing(data.part)) {
            // Request is still ongoing.
            data.progress = mPartInfoProgress / 2;
            mUpdatePartInformationScheduled = true;  // Require reload.
          } else {
            // Request failed.
            data.progress = 0;
          }
        }
      }
      manufacturerItem->setData(Qt::UserRole, QVariant::fromValue(data));

      if (item.isMount() && (!countedParts.contains(row)) && (data.info) &&
          (!data.info->prices.isEmpty())) {
        totalPrice += data.info->getPrice(item.getDesignators().count()) *
            item.getDesignators().count();
        partsWithPrice += item.getDesignators().count();
        countedParts.insert(row);
      }
    }
  }
  mUi->tableWidget->resizeColumnsToContents();

  QString totalStr;
  const int totalParts = mBom->getTotalAssembledPartsCount();
  if (totalPrice > 0) {
    if (partsWithPrice == totalParts) {
      totalStr = tr("%1 parts:").arg(totalParts);
    } else {
      totalStr = tr("%1 of %2 parts:").arg(partsWithPrice).arg(totalParts);
    }
    totalStr += QString(" <b>$ %1</b>").arg(totalPrice, 0, 'f', 2);
  } else {
    totalStr = tr("Total: %1 parts").arg(totalParts);
  }
  mUi->lblTotalPrice->setText(totalStr);
  PartInformationProvider::instance().requestScheduledParts();
}

std::shared_ptr<AssemblyVariant> BomReviewDialog::getAssemblyVariant()
    const noexcept {
  auto uuid = getAssemblyVariantUuid(false);
  return uuid ? mProject.getCircuit().getAssemblyVariants().find(*uuid)
              : std::shared_ptr<AssemblyVariant>();
}

std::optional<Uuid> BomReviewDialog::getAssemblyVariantUuid(
    bool throwIfNullopt) const {
  const std::optional<Uuid> uuid =
      Uuid::tryFromString(mUi->cbxAssemblyVariant->currentData().toString());
  if ((!uuid) && throwIfNullopt) {
    throw LogicError(__FILE__, __LINE__, "No assembly variant selected.");
  }
  return uuid;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
