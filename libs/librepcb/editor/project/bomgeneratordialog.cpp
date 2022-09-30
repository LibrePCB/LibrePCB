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
#include "bomgeneratordialog.h"

#include "../dialogs/filedialog.h"
#include "../workspace/desktopservices.h"
#include "ui_bomgeneratordialog.h"

#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/export/bom.h>
#include <librepcb/core/export/bomcsvwriter.h>
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/bomgenerator.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>

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

BomGeneratorDialog::BomGeneratorDialog(const WorkspaceSettings& settings,
                                       const Project& project,
                                       const Board* board,
                                       QWidget* parent) noexcept
  : QDialog(parent),
    mSettings(settings),
    mProject(project),
    mBom(new Bom(QStringList())),
    mUi(new Ui::BomGeneratorDialog) {
  mUi->setupUi(this);
  mUi->btnBrowse->setFixedWidth(mUi->btnBrowse->sizeHint().height());
  mUi->tableWidget->setWordWrap(false);
  mUi->tableWidget->verticalHeader()->setMinimumSectionSize(10);
  mUi->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mUi->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  mUi->edtOutputPath->setText("./output/{{VERSION}}/{{PROJECT}}_BOM.csv");
  mBtnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::AcceptRole);
  mBtnGenerate->setDefault(true);

  mUi->cbxBoard->addItem(tr("None"));
  foreach (const Board* brd, mProject.getBoards()) {
    mUi->cbxBoard->addItem(*brd->getName());
  }

  int index = board ? mProject.getBoardIndex(*board) + 1 : 0;
  mUi->cbxBoard->setCurrentIndex(index);
  updateBom();

  connect(
      mUi->cbxBoard,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BomGeneratorDialog::updateBom);
  connect(mUi->edtAttributes, &QLineEdit::textEdited, this,
          &BomGeneratorDialog::updateBom);
  connect(mUi->btnBrowse, &QToolButton::clicked, this,
          &BomGeneratorDialog::btnChooseOutputPathClicked);
  connect(mUi->btnBrowseOutputDir, &QPushButton::clicked, this,
          &BomGeneratorDialog::btnOpenOutputDirectoryClicked);
  connect(mBtnGenerate, &QPushButton::clicked, this,
          &BomGeneratorDialog::btnGenerateClicked);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &BomGeneratorDialog::reject);
}

BomGeneratorDialog::~BomGeneratorDialog() noexcept {
}

/*******************************************************************************
 *  GUI Event Handlers
 ******************************************************************************/

void BomGeneratorDialog::btnChooseOutputPathClicked() noexcept {
  QString fp = FileDialog::getSaveFileName(
      this, tr("Save to"), getOutputFilePath().getParentDir().toStr(), "*.csv");
  if (!fp.isEmpty()) {
    mUi->edtOutputPath->setText(fp);
  }
}

void BomGeneratorDialog::btnOpenOutputDirectoryClicked() noexcept {
  DesktopServices ds(mSettings, this);
  ds.openLocalPath(getOutputFilePath().getParentDir());
}

void BomGeneratorDialog::btnGenerateClicked() noexcept {
  try {
    BomCsvWriter writer(*mBom);
    std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
    csv->saveToFile(getOutputFilePath());  // can throw

    QString btnSuccessText = tr("Success!");
    QString btnGenerateText = mBtnGenerate->text();
    if (btnGenerateText != btnSuccessText) {
      mBtnGenerate->setText(btnSuccessText);
      QTimer::singleShot(500, this, [this, btnGenerateText]() {
        if (mBtnGenerate) {
          mBtnGenerate->setText(btnGenerateText);
        }
      });
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BomGeneratorDialog::updateBom() noexcept {
  const Board* board =
      mProject.getBoardByIndex(mUi->cbxBoard->currentIndex() - 1);

  QStringList attributes;
  foreach (const QString str,
           mUi->edtAttributes->text().simplified().split(
               ',', QString::SkipEmptyParts)) {
    attributes.append(str.trimmed());
  }

  BomGenerator gen(mProject);
  gen.setAdditionalAttributes(attributes);
  mBom = gen.generate(board);
  updateTable();

  // Update status label to indicate whether the devices are in sync with the
  // devices chosen in the schematic.
  // See https://github.com/LibrePCB/LibrePCB/issues/584
  if (board) {
    QStringList differentDevices;
    foreach (const BI_Device* device, board->getDeviceInstances()) {
      tl::optional<Uuid> selectedDevice =
          device->getComponentInstance().getDefaultDeviceUuid();
      if (selectedDevice &&
          (selectedDevice != device->getLibDevice().getUuid())) {
        differentDevices.append(*device->getComponentInstance().getName());
      }
    }
    differentDevices.sort(Qt::CaseInsensitive);
    if (!differentDevices.isEmpty()) {
      int max = 20;
      QString designators = differentDevices.mid(0, max).join(", ");
      if (differentDevices.count() > max) {
        designators += ", ...";
      }
      mUi->lblMessage->setText(
          "⚠️ " %
          tr("Warning: %n device(s) from the selected board differ from their "
             "pre-selected devices in the schematic: %1",
             nullptr, differentDevices.count())
              .arg(designators));
      mUi->lblMessage->setStyleSheet(
          "QLabel {background-color: darksalmon; color: black;};");
    } else {
      mUi->lblMessage->setText(
          "✓ " %
          tr("All devices of the selected board are in sync with their "
             "pre-selected devices in the schematic."));
      mUi->lblMessage->setStyleSheet(
          "QLabel {background-color: yellowgreen; color: black;};");
    }
  } else {
    mUi->lblMessage->setText(
        "ℹ️ " %
        tr("Devices are not exported because no board is selected."));
    mUi->lblMessage->setStyleSheet(
        "QLabel {background-color: yellow; color: black;};");
  }
}

void BomGeneratorDialog::updateTable() noexcept {
  mUi->tableWidget->clear();

  try {
    BomCsvWriter writer(*mBom);
    std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
    mUi->tableWidget->setRowCount(csv->getValues().count());
    mUi->tableWidget->setColumnCount(csv->getHeader().count());
    mUi->tableWidget->setHorizontalHeaderLabels(csv->getHeader());
    for (int column = 0; column < csv->getHeader().count(); ++column) {
      mUi->tableWidget->horizontalHeader()->setSectionResizeMode(
          column,
          column <= 1 ? QHeaderView::ResizeToContents : QHeaderView::Stretch);
      for (int row = 0; row < csv->getValues().count(); ++row) {
        QString text = csv->getValues()[row][column];
        text.replace("\n", " ");
        mUi->tableWidget->setItem(row, column, new QTableWidgetItem(text));
      }
    }
    mUi->tableWidget->resizeRowsToContents();
  } catch (Exception& e) {
    qCritical() << "Failed to update BOM table widget:" << e.getMsg();
  }
}

FilePath BomGeneratorDialog::getOutputFilePath() const noexcept {
  QString path = mUi->edtOutputPath->text().trimmed();
  path = AttributeSubstitutor::substitute(
      path, &mProject, [&](const QString& str) {
        return FilePath::cleanFileName(
            str, FilePath::ReplaceSpaces | FilePath::KeepCase);
      });

  if (QDir::isAbsolutePath(path)) {
    return FilePath(path);
  } else {
    return mProject.getPath().getPathTo(path);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
