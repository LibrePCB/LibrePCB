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
#include "fabricationoutputdialog.h"

#include "../../workspace/desktopservices.h"
#include "ui_fabricationoutputdialog.h"

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardfabricationoutputsettings.h>
#include <librepcb/core/project/board/boardgerberexport.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectmetadata.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FabricationOutputDialog::FabricationOutputDialog(
    const WorkspaceSettings& settings, Board& board, QWidget* parent)
  : QDialog(parent),
    mSettings(settings),
    mProject(board.getProject()),
    mBoard(board),
    mUi(new Ui::FabricationOutputDialog) {
  mUi->setupUi(this);
  mBtnGenerate =
      mUi->buttonBox->addButton(tr("&Generate"), QDialogButtonBox::ActionRole);
  connect(mBtnGenerate, &QPushButton::clicked, this,
          &FabricationOutputDialog::btnGenerateClicked);
  connect(mUi->btnDefaultSuffixes, &QPushButton::clicked, this,
          &FabricationOutputDialog::btnDefaultSuffixesClicked);
  connect(mUi->btnProtelSuffixes, &QPushButton::clicked, this,
          &FabricationOutputDialog::btnProtelSuffixesClicked);
  connect(mUi->btnBrowseOutputDir, &QPushButton::clicked, this,
          &FabricationOutputDialog::btnBrowseOutputDirClicked);
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

  QString notes;
  notes += "<p>" %
      tr("This dialog allows to generate Gerber X2 (RS-274X) / Excellon files "
         "for PCB fabrication.") %
      "</p>";
  notes += "<p><b>" %
      tr("Note that it's highly recommended to review the generated files "
         "before ordering PCBs.") %
      "</b><br>";
  notes += tr("This could be done with the free application <a "
              "href=\"%1\">gerbv</a> or the <a href=\"%2\">official reference "
              "viewer from Ucamco</a>.")
               .arg("http://gerbv.geda-project.org/")
               .arg("https://gerber.ucamco.com/") %
      "</p>";
  notes += "<p>" %
      tr("As a simpler and faster alternative, you could use the "
         "<a href=\"%1\">Order PCB</a> feature instead.")
          .arg("order-pcb") %
      "</p>";
  mUi->lblNotes->setText(notes);
  connect(mUi->lblNotes, &QLabel::linkActivated, this,
          [this](const QString& link) {
            if (link == "order-pcb") {
              emit orderPcbDialogTriggered();
            } else {
              DesktopServices ds(mSettings, this);
              ds.openWebUrl(QUrl(link));
            }
          });

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

  // Load window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("fabrication_export_dialog/window_geometry")
          .toByteArray());
}

FabricationOutputDialog::~FabricationOutputDialog() {
  // Save window geometry.
  QSettings clientSettings;
  clientSettings.setValue("fabrication_export_dialog/window_geometry",
                          saveGeometry());
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void FabricationOutputDialog::btnDefaultSuffixesClicked() {
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

void FabricationOutputDialog::btnProtelSuffixesClicked() {
  mUi->edtSuffixOutlines->setText(".gm1");
  mUi->edtSuffixCopperTop->setText(".gtl");
  mUi->edtSuffixCopperInner->setText(".g{{CU_LAYER}}");
  mUi->edtSuffixCopperBot->setText(".gbl");
  mUi->edtSuffixSoldermaskTop->setText(".gts");
  mUi->edtSuffixSoldermaskBot->setText(".gbs");
  mUi->edtSuffixSilkscreenTop->setText(".gto");
  mUi->edtSuffixSilkscreenBot->setText(".gbo");
  mUi->edtSuffixDrillsNpth->setText("_NPTH.drl");
  mUi->edtSuffixDrillsPth->setText("_PTH.drl");
  mUi->edtSuffixDrills->setText(".drl");
  mUi->edtSuffixSolderPasteTop->setText(".gtp");
  mUi->edtSuffixSolderPasteBot->setText(".gbp");
  mUi->cbxDrillsMerge->setChecked(true);
}

void FabricationOutputDialog::btnGenerateClicked() {
  try {
    // Visual feedback with cursor.
    setCursor(Qt::WaitCursor);
    auto cursorScopeGuard = scopeGuard([this]() { unsetCursor(); });

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
    grbExport.exportPcbLayers(mBoard.getFabricationOutputSettings());

    // Show success message.
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
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void FabricationOutputDialog::btnBrowseOutputDirClicked() {
  BoardGerberExport grbExport(mBoard);
  FilePath dir =
      grbExport.getOutputDirectory(mBoard.getFabricationOutputSettings());
  if (dir.isExistingDir()) {
    DesktopServices ds(mSettings, this);
    ds.openLocalPath(dir);
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
}  // namespace librepcb
