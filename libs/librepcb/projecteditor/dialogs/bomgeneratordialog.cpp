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

#include "ui_bomgeneratordialog.h"

#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/common/bom/bom.h>
#include <librepcb/common/bom/bomcsvwriter.h>
#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/common/fileio/csvfile.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/bomgenerator.h>
#include <librepcb/project/project.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BomGeneratorDialog::BomGeneratorDialog(const Project& project,
                                       const Board* board,
                                       QWidget* parent) noexcept
  : QDialog(parent),
    mProject(project),
    mBom(new Bom(QStringList())),
    mUi(new Ui::BomGeneratorDialog) {
  mUi->setupUi(this);
  mUi->lblSuccess->hide();
  mUi->btnBrowse->setFixedWidth(mUi->btnBrowse->sizeHint().height());
  mUi->tableWidget->setWordWrap(false);
  mUi->tableWidget->verticalHeader()->setMinimumSectionSize(10);
  mUi->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mUi->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  mUi->edtOutputPath->setText("./output/{{VERSION}}/{{PROJECT}}_BOM.csv");
  QPushButton* btnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::ActionRole);

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
  connect(mUi->btnOpenDirectory, &QToolButton::clicked, this,
          &BomGeneratorDialog::btnOpenOutputDirectoryClicked);
  connect(btnGenerate, &QPushButton::clicked, this,
          &BomGeneratorDialog::btnGenerateClicked);
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
    mUi->lblSuccess->hide();
  }
}

void BomGeneratorDialog::btnOpenOutputDirectoryClicked() noexcept {
  QDesktopServices::openUrl(
      QUrl::fromLocalFile(getOutputFilePath().getParentDir().toStr()));
}

void BomGeneratorDialog::btnGenerateClicked() noexcept {
  try {
    BomCsvWriter writer(*mBom);
    std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
    csv->saveToFile(getOutputFilePath());  // can throw
    mUi->lblSuccess->show();
  } catch (const Exception& e) {
    mUi->lblSuccess->hide();
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
    mUi->lblSuccess->hide();
  } catch (Exception& e) {
    qCritical() << e.getMsg();
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
}  // namespace project
}  // namespace librepcb
