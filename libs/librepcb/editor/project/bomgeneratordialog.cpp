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
#include "../editorcommandset.h"
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
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>

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
                                       Project& project, const Board* board,
                                       QWidget* parent) noexcept
  : QDialog(parent),
    mSettings(settings),
    mProject(project),
    mBom(new Bom(QStringList())),
    mUi(new Ui::BomGeneratorDialog) {
  mUi->setupUi(this);
  mUi->tableWidget->setWordWrap(false);
  mUi->tableWidget->verticalHeader()->setMinimumSectionSize(10);
  mUi->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mUi->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  const bool multipleAssemblyVariants =
      mProject.getCircuit().getAssemblyVariants().count() > 1;
  mUi->lblAssemblyVariant->setVisible(multipleAssemblyVariants);
  mUi->cbxAssemblyVariant->setVisible(multipleAssemblyVariants);
  QString outPath = "./output/{{VERSION}}/{{PROJECT}}_BOM";
  if (multipleAssemblyVariants) {
    outPath += "_{{VARIANT}}";
  }
  mUi->edtOutputPath->setText(outPath % ".csv");
  mUi->lblNote->setText("ⓘ " % mUi->lblNote->text());
  mBtnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::AcceptRole);
  mBtnGenerate->setDefault(true);

  // Add browse action.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  mUi->edtOutputPath->addAction(
      cmd.inputBrowse.createAction(
          mUi->edtOutputPath, this,
          &BomGeneratorDialog::btnChooseOutputPathClicked,
          EditorCommand::ActionFlag::WidgetShortcut),
      QLineEdit::TrailingPosition);

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

  connect(
      mUi->cbxBoard,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BomGeneratorDialog::updateBom);
  connect(
      mUi->cbxAssemblyVariant,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BomGeneratorDialog::updateBom);
  connect(mUi->edtAttributes, &QLineEdit::textEdited, this,
          &BomGeneratorDialog::updateAttributes);
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

void BomGeneratorDialog::updateAttributes() noexcept {
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

void BomGeneratorDialog::updateBom() noexcept {
  if (auto avUuid = getAssemblyVariantUuid(false)) {
    const Board* board =
        mProject.getBoardByIndex(mUi->cbxBoard->currentIndex() - 1);

    BomGenerator gen(mProject);
    gen.setAdditionalAttributes(mProject.getCustomBomAttributes());
    mBom = gen.generate(board, *avUuid);
    updateTable();
  }
}

void BomGeneratorDialog::updateTable() noexcept {
  mUi->tableWidget->clear();

  try {
    BomCsvWriter writer(*mBom);
    writer.setIncludeNonMountedParts(true);
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
        QTableWidgetItem* item = new QTableWidgetItem(text);
        if (csv->getValues()[row][0] == "0") {
          item->setBackground(Qt::gray);
        }
        mUi->tableWidget->setItem(row, column, item);
      }
    }
    mUi->tableWidget->resizeRowsToContents();
  } catch (Exception& e) {
    qCritical() << "Failed to update BOM table widget:" << e.getMsg();
  }
}

std::shared_ptr<AssemblyVariant> BomGeneratorDialog::getAssemblyVariant() const
    noexcept {
  auto uuid = getAssemblyVariantUuid(false);
  return uuid ? mProject.getCircuit().getAssemblyVariants().find(*uuid)
              : std::shared_ptr<AssemblyVariant>();
}

tl::optional<Uuid> BomGeneratorDialog::getAssemblyVariantUuid(
    bool throwIfNullopt) const {
  const tl::optional<Uuid> uuid =
      Uuid::tryFromString(mUi->cbxAssemblyVariant->currentData().toString());
  if ((!uuid) && throwIfNullopt) {
    throw LogicError(__FILE__, __LINE__, "No assembly variant selected.");
  }
  return uuid;
}

FilePath BomGeneratorDialog::getOutputFilePath() const noexcept {
  QString path = mUi->edtOutputPath->text().trimmed();
  path = AttributeSubstitutor::substitute(
      path, ProjectAttributeLookup(mProject, getAssemblyVariant()),
      [&](const QString& str) {
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
