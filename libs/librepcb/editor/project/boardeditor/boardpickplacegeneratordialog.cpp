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
#include "boardpickplacegeneratordialog.h"

#include "../../workspace/desktopservices.h"
#include "ui_boardpickplacegeneratordialog.h"

#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/export/pickplacecsvwriter.h>
#include <librepcb/core/export/pickplacedata.h>
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardgerberexport.h>
#include <librepcb/core/project/board/boardpickplacegenerator.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPickPlaceGeneratorDialog::BoardPickPlaceGeneratorDialog(
    const WorkspaceSettings& settings, Board& board, QWidget* parent)
  : QDialog(parent),
    mBoard(board),
    mData(),
    mUi(new Ui::BoardPickPlaceGeneratorDialog) {
  mUi->setupUi(this);
  mUi->lblBoardName->setText(*board.getName());
  mUi->tableWidget->setWordWrap(false);
  mUi->tableWidget->verticalHeader()->setMinimumSectionSize(10);
  mUi->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mUi->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  const bool multipleAssemblyVariants =
      mBoard.getProject().getCircuit().getAssemblyVariants().count() > 1;
  mUi->lblAssemblyVariant->setVisible(multipleAssemblyVariants);
  mUi->cbxAssemblyVariant->setVisible(multipleAssemblyVariants);
  QString outPath = "./output/{{VERSION}}/assembly/{{PROJECT}}_PnP";
  if (multipleAssemblyVariants) {
    outPath += "_{{VARIANT}}";
  }
  mUi->edtTopFilePath->setText(outPath % "_TOP.csv");
  mUi->edtBottomFilePath->setText(outPath % "_BOT.csv");
  mUi->lblNote->setText("ⓘ " % mUi->lblNote->text());
  mBtnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::AcceptRole);
  mBtnGenerate->setDefault(true);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &BoardPickPlaceGeneratorDialog::reject);
  connect(mBtnGenerate, &QPushButton::clicked, this,
          &BoardPickPlaceGeneratorDialog::btnGenerateClicked);
  connect(mUi->btnBrowseOutputDir, &QPushButton::clicked, this,
          [this, &settings]() {
            DesktopServices ds(settings, this);
            ds.openLocalPath(
                getOutputFilePath(mUi->edtTopFilePath->text()).getParentDir());
          });
  connect(mUi->rbtnFormatCsvWithMetadata, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) setFileExtension("csv");
          });
  connect(mUi->rbtnFormatCsvWithoutMetadata, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) setFileExtension("csv");
          });
  connect(mUi->rbtnFormatGerberX3, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) setFileExtension("gbr");
          });

  // List assembly variants.
  for (const auto& av :
       mBoard.getProject().getCircuit().getAssemblyVariants()) {
    mUi->cbxAssemblyVariant->addItem(av.getDisplayText(), av.getUuid().toStr());
  }
  mUi->cbxAssemblyVariant->setCurrentIndex(0);
  mUi->cbxAssemblyVariant->setEnabled(mUi->cbxAssemblyVariant->count() > 1);
  connect(
      mUi->cbxAssemblyVariant,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BoardPickPlaceGeneratorDialog::updateData);

  // Load window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("pnp_export_dialog/window_geometry").toByteArray());

  updateData();
}

BoardPickPlaceGeneratorDialog::~BoardPickPlaceGeneratorDialog() {
  // Save window geometry.
  QSettings clientSettings;
  clientSettings.setValue("pnp_export_dialog/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardPickPlaceGeneratorDialog::setFileExtension(
    const QString& extension) noexcept {
  for (QLineEdit* edit : {mUi->edtTopFilePath, mUi->edtBottomFilePath}) {
    QStringList splitPath = edit->text().split(".");
    if ((splitPath.count() > 1) && (splitPath.last().toLower() != extension)) {
      splitPath.last() = extension;
    }
    edit->setText(splitPath.join("."));
  }
}

void BoardPickPlaceGeneratorDialog::btnGenerateClicked() noexcept {
  try {
    if (mUi->rbtnFormatGerberX3->isChecked()) {
      // Gerber X3
      BoardGerberExport gen(mBoard);
      if (mUi->cbxTopDevices->isChecked()) {
        gen.exportComponentLayer(
            BoardGerberExport::BoardSide::Top, *getAssemblyVariantUuid(true),
            getOutputFilePath(mUi->edtTopFilePath->text()));  // can throw
      }
      if (mUi->cbxBottomDevices->isChecked()) {
        gen.exportComponentLayer(
            BoardGerberExport::BoardSide::Bottom, *getAssemblyVariantUuid(true),
            getOutputFilePath(mUi->edtBottomFilePath->text()));  // can throw
      }
    } else {
      // CSV
      PickPlaceCsvWriter writer(*mData);
      writer.setIncludeMetadataComment(
          mUi->rbtnFormatCsvWithMetadata->isChecked());
      if (mUi->cbxTopDevices->isChecked()) {
        writer.setBoardSide(PickPlaceCsvWriter::BoardSide::TOP);
        writer.generateCsv()->saveToFile(
            getOutputFilePath(mUi->edtTopFilePath->text()));  // can throw
      }
      if (mUi->cbxBottomDevices->isChecked()) {
        writer.setBoardSide(PickPlaceCsvWriter::BoardSide::BOTTOM);
        writer.generateCsv()->saveToFile(
            getOutputFilePath(mUi->edtBottomFilePath->text()));  // can throw
      }
    }

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
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardPickPlaceGeneratorDialog::updateData() noexcept {
  mUi->tableWidget->clear();

  try {
    BoardPickPlaceGenerator gen(mBoard, *getAssemblyVariantUuid(true));
    mData = gen.generate();

    PickPlaceCsvWriter writer(*mData);
    writer.setIncludeNonMountedParts(true);
    std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
    mUi->tableWidget->setRowCount(csv->getValues().count());
    mUi->tableWidget->setColumnCount(csv->getHeader().count());
    mUi->tableWidget->setHorizontalHeaderLabels(csv->getHeader());
    for (int column = 0; column < csv->getHeader().count(); ++column) {
      mUi->tableWidget->horizontalHeader()->setSectionResizeMode(
          column,
          (column >= 1) && (column <= 3) ? QHeaderView::Stretch
                                         : QHeaderView::ResizeToContents);
      for (int row = 0; row < csv->getValues().count(); ++row) {
        QString text = csv->getValues()[row][column];
        text.replace("\n", " ");
        QTableWidgetItem* item = new QTableWidgetItem(text);
        if ((row >= mData->getItems().count()) ||
            (!mData->getItems().at(row).isMount())) {
          item->setBackground(Qt::gray);
        }
        mUi->tableWidget->setItem(row, column, item);
      }
    }
    mUi->tableWidget->resizeRowsToContents();
  } catch (Exception& e) {
    qCritical() << "Failed to update pick&place table widget:" << e.getMsg();
  }
}

std::shared_ptr<AssemblyVariant>
    BoardPickPlaceGeneratorDialog::getAssemblyVariant() const noexcept {
  auto uuid = getAssemblyVariantUuid(false);
  return uuid
      ? mBoard.getProject().getCircuit().getAssemblyVariants().find(*uuid)
      : std::shared_ptr<AssemblyVariant>();
}

tl::optional<Uuid> BoardPickPlaceGeneratorDialog::getAssemblyVariantUuid(
    bool throwIfNullopt) const {
  const tl::optional<Uuid> uuid =
      Uuid::tryFromString(mUi->cbxAssemblyVariant->currentData().toString());
  if ((!uuid) && throwIfNullopt) {
    throw LogicError(__FILE__, __LINE__, "No assembly variant selected.");
  }
  return uuid;
}

FilePath BoardPickPlaceGeneratorDialog::getOutputFilePath(
    const QString& text) const noexcept {
  QString path = AttributeSubstitutor::substitute(
      text.trimmed(), ProjectAttributeLookup(mBoard, getAssemblyVariant()),
      [&](const QString& str) {
        return FilePath::cleanFileName(
            str, FilePath::ReplaceSpaces | FilePath::KeepCase);
      });

  if (path.isEmpty()) {
    return FilePath();
  } else if (QDir::isAbsolutePath(path)) {
    return FilePath(path);
  } else {
    return mBoard.getProject().getPath().getPathTo(path);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
