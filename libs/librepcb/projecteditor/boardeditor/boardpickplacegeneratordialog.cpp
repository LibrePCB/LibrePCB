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

#include "ui_boardpickplacegeneratordialog.h"

#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/common/fileio/csvfile.h>
#include <librepcb/common/pnp/pickplacecsvwriter.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardpickplacegenerator.h>
#include <librepcb/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPickPlaceGeneratorDialog::BoardPickPlaceGeneratorDialog(Board&   board,
                                                             QWidget* parent)
  : QDialog(parent),
    mBoard(board),
    mData(),
    mUi(new Ui::BoardPickPlaceGeneratorDialog) {
  mUi->setupUi(this);
  mUi->lblSuccess->hide();
  mUi->lblBoardName->setText(*board.getName());
  mUi->tableWidget->setWordWrap(false);
  mUi->tableWidget->verticalHeader()->setMinimumSectionSize(10);
  mUi->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mUi->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  mUi->edtTopFilePath->setText(
      "./output/{{VERSION}}/assembly/{{PROJECT}}_PnP-TOP.csv");
  mUi->edtBottomFilePath->setText(
      "./output/{{VERSION}}/assembly/{{PROJECT}}_PnP-BOT.csv");
  QPushButton* btnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::ActionRole);
  connect(btnGenerate, &QPushButton::clicked, this,
          &BoardPickPlaceGeneratorDialog::btnGenerateClicked);

  BoardPickPlaceGenerator gen(mBoard);
  mData = gen.generate();
  updateTable();
}

BoardPickPlaceGeneratorDialog::~BoardPickPlaceGeneratorDialog() {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardPickPlaceGeneratorDialog::btnGenerateClicked() noexcept {
  try {
    PickPlaceCsvWriter writer(*mData);
    writer.setIncludeMetadataComment(mUi->cbxIncludeComment->isChecked());
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
    mUi->lblSuccess->show();
  } catch (Exception& e) {
    mUi->lblSuccess->hide();
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardPickPlaceGeneratorDialog::updateTable() noexcept {
  mUi->tableWidget->clear();

  try {
    PickPlaceCsvWriter       writer(*mData);
    std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
    mUi->tableWidget->setRowCount(csv->getValues().count());
    mUi->tableWidget->setColumnCount(csv->getHeader().count());
    mUi->tableWidget->setHorizontalHeaderLabels(csv->getHeader());
    for (int column = 0; column < csv->getHeader().count(); ++column) {
      mUi->tableWidget->horizontalHeader()->setSectionResizeMode(
          column, (column >= 1) && (column <= 3)
                      ? QHeaderView::Stretch
                      : QHeaderView::ResizeToContents);
      for (int row = 0; row < csv->getValues().count(); ++row) {
        QString text = csv->getValues()[row][column];
        text.replace("\n", " ");
        mUi->tableWidget->setItem(row, column, new QTableWidgetItem(text));
      }
    }
    mUi->tableWidget->resizeRowsToContents();
  } catch (Exception& e) {
    qCritical() << e.getMsg();
  }
}

FilePath BoardPickPlaceGeneratorDialog::getOutputFilePath(
    const QString& text) const noexcept {
  QString path = AttributeSubstitutor::substitute(
      text.trimmed(), &mBoard.getProject(), [&](const QString& str) {
        return FilePath::cleanFileName(
            str, FilePath::ReplaceSpaces | FilePath::KeepCase);
      });

  if (QDir::isAbsolutePath(path)) {
    return FilePath(path);
  } else {
    return mBoard.getProject().getPath().getPathTo(path);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
