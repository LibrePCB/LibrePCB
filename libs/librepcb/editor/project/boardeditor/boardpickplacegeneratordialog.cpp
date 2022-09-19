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
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardgerberexport.h>
#include <librepcb/core/project/board/boardpickplacegenerator.h>
#include <librepcb/core/project/project.h>

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
  mUi->edtTopFilePath->setText(
      "./output/{{VERSION}}/assembly/{{PROJECT}}_PnP-TOP.csv");
  mUi->edtBottomFilePath->setText(
      "./output/{{VERSION}}/assembly/{{PROJECT}}_PnP-BOT.csv");
  mBtnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::ActionRole);
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

  // Load window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("pnp_export_dialog/window_geometry").toByteArray());

  BoardPickPlaceGenerator gen(mBoard);
  mData = gen.generate();
  updateTable();
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
            BoardGerberExport::BoardSide::Top,
            getOutputFilePath(mUi->edtTopFilePath->text()));  // can throw
      }
      if (mUi->cbxBottomDevices->isChecked()) {
        gen.exportComponentLayer(
            BoardGerberExport::BoardSide::Bottom,
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

void BoardPickPlaceGeneratorDialog::updateTable() noexcept {
  mUi->tableWidget->clear();

  try {
    PickPlaceCsvWriter writer(*mData);
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
        mUi->tableWidget->setItem(row, column, new QTableWidgetItem(text));
      }
    }
    mUi->tableWidget->resizeRowsToContents();
  } catch (Exception& e) {
    qCritical() << "Failed to update pick&place table widget:" << e.getMsg();
  }
}

FilePath BoardPickPlaceGeneratorDialog::getOutputFilePath(
    const QString& text) const noexcept {
  QString path = AttributeSubstitutor::substitute(
      text.trimmed(), &mBoard.getProject(), [&](const QString& str) {
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
