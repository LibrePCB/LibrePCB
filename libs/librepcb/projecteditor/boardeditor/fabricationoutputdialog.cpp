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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fabricationoutputdialog.h"

#include "ui_fabricationoutputdialog.h"

#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardfabricationoutputsettings.h>
#include <librepcb/project/boards/boardgerberexport.h>
#include <librepcb/project/metadata/projectmetadata.h>
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

FabricationOutputDialog::FabricationOutputDialog(Board& board, QWidget* parent)
  : QDialog(parent),
    mProject(board.getProject()),
    mBoard(board),
    mUi(new Ui::FabricationOutputDialog) {
  mUi->setupUi(this);
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, mUi->edtSuffixDrills,
          &QLineEdit::setEnabled);
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, mUi->edtSuffixDrillsNpth,
          &QLineEdit::setDisabled);
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, mUi->edtSuffixDrillsPth,
          &QLineEdit::setDisabled);
  connect(mUi->cbxSolderPasteTop, &QCheckBox::toggled,
          mUi->edtSuffixSolderPasteTop, &QLineEdit::setEnabled);
  connect(mUi->cbxSolderPasteBot, &QCheckBox::toggled,
          mUi->edtSuffixSolderPasteBot, &QLineEdit::setEnabled);

  BoardFabricationOutputSettings s = mBoard.getFabricationOutputSettings();
  mUi->edtBasePath->setText(s.getOutputBasePath());
  mUi->edtSuffixOutlines->setText(s.getSuffixOutlines());
  mUi->edtSuffixCopperTop->setText(s.getSuffixCopperTop());
  mUi->edtSuffixCopperInner->setText(s.getSuffixCopperInner());
  mUi->edtSuffixCopperBot->setText(s.getSuffixCopperBot());
  mUi->edtSuffixSoldermaskTop->setText(s.getSuffixSolderMaskTop());
  mUi->edtSuffixSoldermaskBot->setText(s.getSuffixSolderMaskBot());
  mUi->edtSuffixSilkscreenTop->setText(s.getSuffixSilkscreenTop());
  mUi->edtSuffixSilkscreenBot->setText(s.getSuffixSilkscreenBot());
  mUi->edtSuffixDrillsNpth->setText(s.getSuffixDrillsNpth());
  mUi->edtSuffixDrillsPth->setText(s.getSuffixDrillsPth());
  mUi->edtSuffixDrills->setText(s.getSuffixDrills());
  mUi->edtSuffixSolderPasteTop->setText(s.getSuffixSolderPasteTop());
  mUi->edtSuffixSolderPasteBot->setText(s.getSuffixSolderPasteBot());
  mUi->cbxDrillsMerge->setChecked(s.getMergeDrillFiles());
  mUi->cbxSolderPasteTop->setChecked(s.getEnableSolderPasteTop());
  mUi->cbxSolderPasteBot->setChecked(s.getEnableSolderPasteBot());

  QStringList topSilkscreen = s.getSilkscreenLayersTop();
  mUi->cbxSilkTopPlacement->setChecked(
      topSilkscreen.contains(GraphicsLayer::sTopPlacement));
  mUi->cbxSilkTopNames->setChecked(
      topSilkscreen.contains(GraphicsLayer::sTopNames));
  mUi->cbxSilkTopValues->setChecked(
      topSilkscreen.contains(GraphicsLayer::sTopValues));

  QStringList botSilkscreen = s.getSilkscreenLayersBot();
  mUi->cbxSilkBotPlacement->setChecked(
      botSilkscreen.contains(GraphicsLayer::sBotPlacement));
  mUi->cbxSilkBotNames->setChecked(
      botSilkscreen.contains(GraphicsLayer::sBotNames));
  mUi->cbxSilkBotValues->setChecked(
      botSilkscreen.contains(GraphicsLayer::sBotValues));
}

FabricationOutputDialog::~FabricationOutputDialog() {
  delete mUi;
  mUi = nullptr;
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void FabricationOutputDialog::on_btnDefaultSuffixes_clicked() {
  mUi->edtSuffixOutlines->setText("_OUTLINES.gbr");
  mUi->edtSuffixCopperTop->setText("_COPPER-TOP.gbr");
  mUi->edtSuffixCopperInner->setText("_COPPER-IN{{CU_LAYER}}.gbr");
  mUi->edtSuffixCopperBot->setText("_COPPER-BOTTOM.gbr");
  mUi->edtSuffixSoldermaskTop->setText("_SOLDERMASK-TOP.gbr");
  mUi->edtSuffixSoldermaskBot->setText("_SOLDERMASK-BOTTOM.gbr");
  mUi->edtSuffixSilkscreenTop->setText("_SILKSCREEN-TOP.gbr");
  mUi->edtSuffixSilkscreenBot->setText("_SILKSCREEN-BOTTOM.gbr");
  mUi->edtSuffixDrillsNpth->setText("_DRILLS-NPTH.drl");
  mUi->edtSuffixDrillsPth->setText("_DRILLS-PTH.drl");
  mUi->edtSuffixDrills->setText("_DRILLS.drl");
  mUi->edtSuffixSolderPasteTop->setText("_SOLDERPASTE-TOP.gbr");
  mUi->edtSuffixSolderPasteBot->setText("_SOLDERPASTE-BOTTOM.gbr");
  mUi->cbxDrillsMerge->setChecked(false);
}

void FabricationOutputDialog::on_btnProtelSuffixes_clicked() {
  mUi->edtSuffixOutlines->setText(".gm1");
  mUi->edtSuffixCopperTop->setText(".gtl");
  mUi->edtSuffixCopperInner->setText(".g{{CU_LAYER}}");
  mUi->edtSuffixCopperBot->setText(".gbl");
  mUi->edtSuffixSoldermaskTop->setText(".gts");
  mUi->edtSuffixSoldermaskBot->setText(".gbs");
  mUi->edtSuffixSilkscreenTop->setText(".gto");
  mUi->edtSuffixSilkscreenBot->setText(".gbo");
  mUi->edtSuffixDrillsNpth->setText("_NPTH.txt");
  mUi->edtSuffixDrillsPth->setText("_PTH.txt");
  mUi->edtSuffixDrills->setText(".txt");
  mUi->edtSuffixSolderPasteTop->setText(".gtp");
  mUi->edtSuffixSolderPasteBot->setText(".gbp");
  mUi->cbxDrillsMerge->setChecked(true);
}

void FabricationOutputDialog::on_btnGenerate_clicked() {
  try {
    // rebuild planes because they may be outdated!
    mBoard.rebuildAllPlanes();

    // update fabrication output settings if modified
    BoardFabricationOutputSettings s = mBoard.getFabricationOutputSettings();
    s.setOutputBasePath(mUi->edtBasePath->text().trimmed());
    s.setSuffixDrills(mUi->edtSuffixDrills->text().trimmed());
    s.setSuffixDrillsNpth(mUi->edtSuffixDrillsNpth->text().trimmed());
    s.setSuffixDrillsPth(mUi->edtSuffixDrillsPth->text().trimmed());
    s.setSuffixOutlines(mUi->edtSuffixOutlines->text().trimmed());
    s.setSuffixCopperTop(mUi->edtSuffixCopperTop->text().trimmed());
    s.setSuffixCopperInner(mUi->edtSuffixCopperInner->text().trimmed());
    s.setSuffixCopperBot(mUi->edtSuffixCopperBot->text().trimmed());
    s.setSuffixSolderMaskTop(mUi->edtSuffixSoldermaskTop->text().trimmed());
    s.setSuffixSolderMaskBot(mUi->edtSuffixSoldermaskBot->text().trimmed());
    s.setSuffixSilkscreenTop(mUi->edtSuffixSilkscreenTop->text().trimmed());
    s.setSuffixSilkscreenBot(mUi->edtSuffixSilkscreenBot->text().trimmed());
    s.setSuffixSolderPasteTop(mUi->edtSuffixSolderPasteTop->text().trimmed());
    s.setSuffixSolderPasteBot(mUi->edtSuffixSolderPasteBot->text().trimmed());
    s.setSilkscreenLayersTop(getTopSilkscreenLayers());
    s.setSilkscreenLayersBot(getBotSilkscreenLayers());
    s.setMergeDrillFiles(mUi->cbxDrillsMerge->isChecked());
    s.setEnableSolderPasteTop(mUi->cbxSolderPasteTop->isChecked());
    s.setEnableSolderPasteBot(mUi->cbxSolderPasteBot->isChecked());
    if (s != mBoard.getFabricationOutputSettings()) {
      mBoard.getFabricationOutputSettings() = s;  // TODO: use undo command
    }

    // generate files
    BoardGerberExport grbExport(mBoard);
    grbExport.exportAllLayers();
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void FabricationOutputDialog::on_btnBrowseOutputDir_clicked() {
  BoardGerberExport grbExport(mBoard);
  FilePath          dir = grbExport.getOutputDirectory();
  if (dir.isExistingDir()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.toStr()));
  } else {
    QMessageBox::warning(this, tr("Warning"), tr("Directory does not exist."));
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QStringList FabricationOutputDialog::getTopSilkscreenLayers() const noexcept {
  QStringList layers;
  if (mUi->cbxSilkTopPlacement->isChecked()) {
    layers << GraphicsLayer::sTopPlacement;
  }
  if (mUi->cbxSilkTopNames->isChecked()) {
    layers << GraphicsLayer::sTopNames;
  }
  if (mUi->cbxSilkTopValues->isChecked()) {
    layers << GraphicsLayer::sTopValues;
  }
  return layers;
}

QStringList FabricationOutputDialog::getBotSilkscreenLayers() const noexcept {
  QStringList layers;
  if (mUi->cbxSilkBotPlacement->isChecked()) {
    layers << GraphicsLayer::sBotPlacement;
  }
  if (mUi->cbxSilkBotNames->isChecked()) {
    layers << GraphicsLayer::sBotNames;
  }
  if (mUi->cbxSilkBotValues->isChecked()) {
    layers << GraphicsLayer::sBotValues;
  }
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
