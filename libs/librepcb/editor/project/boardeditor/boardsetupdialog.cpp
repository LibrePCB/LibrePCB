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
#include "boardsetupdialog.h"

#include "../../undostack.h"
#include "../cmd/cmdboardedit.h"
#include "ui_boardsetupdialog.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/drc/boarddesignrulechecksettings.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/types/pcbcolor.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

const QString BoardSetupDialog::sSettingsPrefix =
    "board_editor/board_setup_dialog";

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardSetupDialog::BoardSetupDialog(Board& board, UndoStack& undoStack,
                                   QWidget* parent) noexcept
  : QDialog(parent),
    mBoard(board),
    mUndoStack(undoStack),
    mUi(new Ui::BoardSetupDialog) {
  mUi->setupUi(this);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &BoardSetupDialog::buttonBoxClicked);

  // Tab: General
  mUi->spbxInnerCopperLayerCount->setMinimum(0);
  mUi->spbxInnerCopperLayerCount->setMaximum(Layer::innerCopperCount());
  mUi->edtPcbThickness->setToolTip(tr("Default:") % " 1.6 mm");
  mUi->edtPcbThickness->configure(mBoard.getGridUnit(),
                                  LengthEditBase::Steps::generic(),
                                  sSettingsPrefix % "/pcb_thickness");
  mUi->cbxSolderResist->addItem(
      tr("None (fully exposed copper)"),
      QVariant::fromValue(static_cast<const PcbColor*>(nullptr)));
  foreach (const PcbColor* color, PcbColor::all()) {
    const QString defaultSuffix = " (" % tr("default") % ")";
    if (color->isAvailableForSolderResist()) {
      QString text = color->getNameTr();
      if (color == &PcbColor::green()) text += defaultSuffix;
      mUi->cbxSolderResist->addItem(text, QVariant::fromValue(color));
    }
    if (color->isAvailableForSilkscreen()) {
      QString text = color->getNameTr();
      if (color == &PcbColor::white()) text += defaultSuffix;
      mUi->cbxSilkscreenColor->addItem(text, QVariant::fromValue(color));
    }
  }
  for (QLabel* lbl : {
           mUi->lblInnerLayers,
           mUi->lblPcbThickness,
           mUi->lblSolderResist,
           mUi->lblSilkscreenColor,
       }) {
    lbl->setText(lbl->text().replace(":", "") % "*:");
  }
  mUi->lblNoteAboutSettingsHandover->setText(
      "*) " % mUi->lblNoteAboutSettingsHandover->text());
  mUi->cbxSilkTopLegend->setText(Layer::topLegend().getNameTr());
  mUi->cbxSilkTopNames->setText(Layer::topNames().getNameTr());
  mUi->cbxSilkTopValues->setText(Layer::topValues().getNameTr());
  mUi->cbxSilkBotLegend->setText(Layer::botLegend().getNameTr());
  mUi->cbxSilkBotNames->setText(Layer::botNames().getNameTr());
  mUi->cbxSilkBotValues->setText(Layer::botValues().getNameTr());

  // Tab: Design Rules
  mUi->edtRulesStopMaskClrRatio->setSingleStep(5.0);  // [%]
  mUi->edtRulesStopMaskClrMin->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/stopmask_clearance_min");
  mUi->edtRulesStopMaskClrMax->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/stopmask_clearance_max");
  mUi->edtRulesSolderPasteClrRatio->setSingleStep(5.0);  // [%]
  mUi->edtRulesSolderPasteClrMin->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/solderpaste_clearance_min");
  mUi->edtRulesSolderPasteClrMax->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/solderpaste_clearance_max");
  mUi->edtRulesPadAnnularRingRatio->setSingleStep(5.0);  // [%]
  mUi->edtRulesPadAnnularRingMin->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/pad_annular_ring_min");
  mUi->edtRulesPadAnnularRingMax->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/pad_annular_ring_max");
  mUi->edtRulesViaAnnularRingRatio->setSingleStep(5.0);  // [%]
  mUi->edtRulesViaAnnularRingMin->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/via_annular_ring_min");
  mUi->edtRulesViaAnnularRingMax->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/via_annular_ring_max");
  mUi->edtRulesStopMaskMaxViaDia->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/stopmask_max_via_diameter");
  for (auto rbtn :
       {mUi->rbtnRulesCmpSidePadFullShape, mUi->rbtnRulesInnerPadFullShape}) {
    rbtn->setToolTip(
        tr("<p>Always use the full pad shape as defined in the footprint from "
           "the library.</p><p>This is the safer and thus preferred option, "
           "but requires more space for the pads.</p>"));
  }
  for (auto rbtn : {mUi->rbtnRulesCmpSidePadAutoAnnular,
                    mUi->rbtnRulesInnerPadAutoAnnular}) {
    rbtn->setToolTip(
        tr("<p>Don't use the defined pad shape, but automatic annular rings "
           "calculated by the parameters below. The annular ring of "
           "unconnected pads is reduced to the specified mimimum value.</p>"
           "<p>This option is more space-efficient, but works only reliable "
           "if the entered parameters comply with the PCB manufacturers "
           "capabilities.</p>"));
  }
  mUi->lblRulesCmpSidePadWarning->setVisible(
      mUi->rbtnRulesCmpSidePadAutoAnnular->isChecked());
  connect(mUi->rbtnRulesCmpSidePadAutoAnnular, &QRadioButton::toggled,
          mUi->lblRulesCmpSidePadWarning, &QLabel::setVisible);
  connect(mUi->edtRulesStopMaskClrMin, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesStopMaskClrMax, &UnsignedLengthEdit::clipToMinimum);
  connect(mUi->edtRulesStopMaskClrMax, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesStopMaskClrMin, &UnsignedLengthEdit::clipToMaximum);
  connect(mUi->edtRulesSolderPasteClrMin, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesSolderPasteClrMax, &UnsignedLengthEdit::clipToMinimum);
  connect(mUi->edtRulesSolderPasteClrMax, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesSolderPasteClrMin, &UnsignedLengthEdit::clipToMaximum);
  connect(mUi->edtRulesPadAnnularRingMin, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesPadAnnularRingMax, &UnsignedLengthEdit::clipToMinimum);
  connect(mUi->edtRulesPadAnnularRingMax, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesPadAnnularRingMin, &UnsignedLengthEdit::clipToMaximum);
  connect(mUi->edtRulesViaAnnularRingMin, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesViaAnnularRingMax, &UnsignedLengthEdit::clipToMinimum);
  connect(mUi->edtRulesViaAnnularRingMax, &UnsignedLengthEdit::valueChanged,
          mUi->edtRulesViaAnnularRingMin, &UnsignedLengthEdit::clipToMaximum);

  // Tab: DRC Settings
  mUi->edtDrcClearanceCopperCopper->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/clearance_copper_copper");
  mUi->edtDrcClearanceCopperBoard->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/clearance_copper_board");
  mUi->edtDrcClearanceCopperNpth->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/clearance_copper_npth");
  mUi->edtDrcClearanceDrillDrill->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/clearance_drill_drill");
  mUi->edtDrcClearanceDrillBoard->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/clearance_drill_board");
  mUi->edtDrcClearanceSilkscreenStopmask->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/clearance_silkscreen_stopmask");
  mUi->edtDrcMinCopperWidth->configure(mBoard.getGridUnit(),
                                       LengthEditBase::Steps::generic(),
                                       sSettingsPrefix % "/min_copper_width");
  mUi->edtDrcMinPthAnnularRing->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/min_pth_annular_ring");
  mUi->edtDrcMinNpthDrillDiameter->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::drillDiameter(),
      sSettingsPrefix % "/min_npth_drill_diameter");
  mUi->edtDrcMinNpthSlotWidth->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::drillDiameter(),
      sSettingsPrefix % "/min_npth_slot_width");
  mUi->edtDrcMinPthDrillDiameter->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::drillDiameter(),
      sSettingsPrefix % "/min_pth_drill_diameter");
  mUi->edtDrcMinPthSlotWidth->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::drillDiameter(),
      sSettingsPrefix % "/min_pth_slot_width");
  mUi->edtDrcMinSilkscreenWidth->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/min_silkscreen_width");
  mUi->edtDrcMinSilkscreenTextHeight->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::generic(),
      sSettingsPrefix % "/min_silkscreen_text_height");
  mUi->edtDrcMinOutlineToolDiameter->configure(
      mBoard.getGridUnit(), LengthEditBase::Steps::drillDiameter(),
      sSettingsPrefix % "/min_outline_tool_diameter");
  for (QComboBox* cbx :
       {mUi->cbxDrcAllowedNpthSlots, mUi->cbxDrcAllowedPthSlots}) {
    cbx->addItem(
        tr("None"),
        QVariant::fromValue(BoardDesignRuleCheckSettings::AllowedSlots::None));
    cbx->addItem(
        tr("Only Simple Oblongs"),
        QVariant::fromValue(
            BoardDesignRuleCheckSettings::AllowedSlots::SingleSegmentStraight));
    cbx->addItem(
        tr("Any Without Curves"),
        QVariant::fromValue(
            BoardDesignRuleCheckSettings::AllowedSlots::MultiSegmentStraight));
    cbx->addItem(
        tr("Any"),
        QVariant::fromValue(BoardDesignRuleCheckSettings::AllowedSlots::Any));
  }

  // Load all s.
  load();

  // Load the window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value(sSettingsPrefix % "/window_geometry").toByteArray());

  // Always open first tab.
  mUi->tabWidget->setCurrentIndex(0);
}

BoardSetupDialog::~BoardSetupDialog() {
  // Save the window geometry.
  QSettings clientSettings;
  clientSettings.setValue(sSettingsPrefix % "/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardSetupDialog::openDrcSettingsTab() noexcept {
  mUi->tabWidget->setCurrentWidget(mUi->tabDrcSettings);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardSetupDialog::buttonBoxClicked(QAbstractButton* button) {
  Q_ASSERT(button);
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      apply();
      break;
    case QDialogButtonBox::AcceptRole:
      if (apply()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      break;
  }
}

void BoardSetupDialog::load() noexcept {
  // Tab: General
  mUi->edtBoardName->setText(*mBoard.getName());
  mUi->spbxInnerCopperLayerCount->setValue(mBoard.getInnerLayerCount());
  mUi->edtPcbThickness->setValue(mBoard.getPcbThickness());
  mUi->cbxSolderResist->setCurrentIndex(mUi->cbxSolderResist->findData(
      QVariant::fromValue(mBoard.getSolderResist())));
  mUi->cbxSilkscreenColor->setCurrentIndex(mUi->cbxSilkscreenColor->findData(
      QVariant::fromValue(&mBoard.getSilkscreenColor())));
  const QVector<const Layer*>& topLegend = mBoard.getSilkscreenLayersTop();
  mUi->cbxSilkTopLegend->setChecked(topLegend.contains(&Layer::topLegend()));
  mUi->cbxSilkTopNames->setChecked(topLegend.contains(&Layer::topNames()));
  mUi->cbxSilkTopValues->setChecked(topLegend.contains(&Layer::topValues()));
  const QVector<const Layer*>& botLegend = mBoard.getSilkscreenLayersBot();
  mUi->cbxSilkBotLegend->setChecked(botLegend.contains(&Layer::botLegend()));
  mUi->cbxSilkBotNames->setChecked(botLegend.contains(&Layer::botNames()));
  mUi->cbxSilkBotValues->setChecked(botLegend.contains(&Layer::botValues()));

  // Tab: Design Rules
  const BoardDesignRules& r = mBoard.getDesignRules();
  mUi->edtRulesStopMaskClrRatio->setValue(r.getStopMaskClearance().getRatio());
  mUi->edtRulesStopMaskClrMin->setValue(r.getStopMaskClearance().getMinValue());
  mUi->edtRulesStopMaskClrMax->setValue(r.getStopMaskClearance().getMaxValue());
  mUi->edtRulesSolderPasteClrRatio->setValue(
      r.getSolderPasteClearance().getRatio());
  mUi->edtRulesSolderPasteClrMin->setValue(
      r.getSolderPasteClearance().getMinValue());
  mUi->edtRulesSolderPasteClrMax->setValue(
      r.getSolderPasteClearance().getMaxValue());
  if (r.getPadCmpSideAutoAnnularRing()) {
    mUi->rbtnRulesCmpSidePadAutoAnnular->setChecked(true);
  } else {
    mUi->rbtnRulesCmpSidePadFullShape->setChecked(true);
  }
  if (r.getPadInnerAutoAnnularRing()) {
    mUi->rbtnRulesInnerPadAutoAnnular->setChecked(true);
  } else {
    mUi->rbtnRulesInnerPadFullShape->setChecked(true);
  }
  mUi->edtRulesPadAnnularRingRatio->setValue(r.getPadAnnularRing().getRatio());
  mUi->edtRulesPadAnnularRingMin->setValue(r.getPadAnnularRing().getMinValue());
  mUi->edtRulesPadAnnularRingMax->setValue(r.getPadAnnularRing().getMaxValue());
  mUi->edtRulesViaAnnularRingRatio->setValue(r.getViaAnnularRing().getRatio());
  mUi->edtRulesViaAnnularRingMin->setValue(r.getViaAnnularRing().getMinValue());
  mUi->edtRulesViaAnnularRingMax->setValue(r.getViaAnnularRing().getMaxValue());
  mUi->edtRulesStopMaskMaxViaDia->setValue(r.getStopMaskMaxViaDiameter());

  // Tab: DRC Settings
  mUi->edtDrcClearanceCopperCopper->setValue(
      mBoard.getDrcSettings().getMinCopperCopperClearance());
  mUi->edtDrcClearanceCopperBoard->setValue(
      mBoard.getDrcSettings().getMinCopperBoardClearance());
  mUi->edtDrcClearanceCopperNpth->setValue(
      mBoard.getDrcSettings().getMinCopperNpthClearance());
  mUi->edtDrcClearanceDrillDrill->setValue(
      mBoard.getDrcSettings().getMinDrillDrillClearance());
  mUi->edtDrcClearanceDrillBoard->setValue(
      mBoard.getDrcSettings().getMinDrillBoardClearance());
  mUi->edtDrcClearanceSilkscreenStopmask->setValue(
      mBoard.getDrcSettings().getMinSilkscreenStopmaskClearance());
  mUi->edtDrcMinCopperWidth->setValue(
      mBoard.getDrcSettings().getMinCopperWidth());
  mUi->edtDrcMinPthAnnularRing->setValue(
      mBoard.getDrcSettings().getMinPthAnnularRing());
  mUi->edtDrcMinNpthDrillDiameter->setValue(
      mBoard.getDrcSettings().getMinNpthDrillDiameter());
  mUi->edtDrcMinNpthSlotWidth->setValue(
      mBoard.getDrcSettings().getMinNpthSlotWidth());
  mUi->edtDrcMinPthDrillDiameter->setValue(
      mBoard.getDrcSettings().getMinPthDrillDiameter());
  mUi->edtDrcMinPthSlotWidth->setValue(
      mBoard.getDrcSettings().getMinPthSlotWidth());
  mUi->edtDrcMinSilkscreenWidth->setValue(
      mBoard.getDrcSettings().getMinSilkscreenWidth());
  mUi->edtDrcMinSilkscreenTextHeight->setValue(
      mBoard.getDrcSettings().getMinSilkscreenTextHeight());
  mUi->edtDrcMinOutlineToolDiameter->setValue(
      mBoard.getDrcSettings().getMinOutlineToolDiameter());
  mUi->cbxBlindViasAllowed->setChecked(
      mBoard.getDrcSettings().getBlindViasAllowed());
  mUi->cbxBuriedViasAllowed->setChecked(
      mBoard.getDrcSettings().getBuriedViasAllowed());
  mUi->cbxDrcAllowedNpthSlots->setCurrentIndex(
      mUi->cbxDrcAllowedNpthSlots->findData(
          QVariant::fromValue(mBoard.getDrcSettings().getAllowedNpthSlots())));
  mUi->cbxDrcAllowedPthSlots->setCurrentIndex(
      mUi->cbxDrcAllowedPthSlots->findData(
          QVariant::fromValue(mBoard.getDrcSettings().getAllowedPthSlots())));
}

bool BoardSetupDialog::apply() noexcept {
  try {
    QScopedPointer<CmdBoardEdit> cmd(new CmdBoardEdit(mBoard));

    // Tab: General
    cmd->setName(
        ElementName(mUi->edtBoardName->text().trimmed()));  // can throw
    cmd->setInnerLayerCount(mUi->spbxInnerCopperLayerCount->value());
    cmd->setPcbThickness(mUi->edtPcbThickness->getValue());
    if (mUi->cbxSolderResist->currentIndex() >= 0) {
      cmd->setSolderResist(
          mUi->cbxSolderResist->currentData().value<const PcbColor*>());
    }
    if (const PcbColor* color =
            mUi->cbxSilkscreenColor->currentData().value<const PcbColor*>()) {
      cmd->setSilkscreenColor(*color);
    }
    cmd->setSilkscreenLayersTop(getTopSilkscreenLayers());
    cmd->setSilkscreenLayersBot(getBotSilkscreenLayers());

    // Tab: Design Rules
    BoardDesignRules r = mBoard.getDesignRules();
    r.setStopMaskClearance(BoundedUnsignedRatio(
        mUi->edtRulesStopMaskClrRatio->getValue(),
        mUi->edtRulesStopMaskClrMin->getValue(),
        mUi->edtRulesStopMaskClrMax->getValue()));  // can throw
    r.setSolderPasteClearance(BoundedUnsignedRatio(
        mUi->edtRulesSolderPasteClrRatio->getValue(),
        mUi->edtRulesSolderPasteClrMin->getValue(),
        mUi->edtRulesSolderPasteClrMax->getValue()));  // can throw
    r.setPadCmpSideAutoAnnularRing(
        mUi->rbtnRulesCmpSidePadAutoAnnular->isChecked());
    r.setPadInnerAutoAnnularRing(
        mUi->rbtnRulesInnerPadAutoAnnular->isChecked());
    r.setPadAnnularRing(BoundedUnsignedRatio(
        mUi->edtRulesPadAnnularRingRatio->getValue(),
        mUi->edtRulesPadAnnularRingMin->getValue(),
        mUi->edtRulesPadAnnularRingMax->getValue()));  // can throw
    r.setViaAnnularRing(BoundedUnsignedRatio(
        mUi->edtRulesViaAnnularRingRatio->getValue(),
        mUi->edtRulesViaAnnularRingMin->getValue(),
        mUi->edtRulesViaAnnularRingMax->getValue()));  // can throw
    r.setStopMaskMaxViaDiameter(mUi->edtRulesStopMaskMaxViaDia->getValue());
    cmd->setDesignRules(r);

    // Tab: DRC Settings
    BoardDesignRuleCheckSettings s = mBoard.getDrcSettings();
    s.setMinCopperCopperClearance(mUi->edtDrcClearanceCopperCopper->getValue());
    s.setMinCopperBoardClearance(mUi->edtDrcClearanceCopperBoard->getValue());
    s.setMinCopperNpthClearance(mUi->edtDrcClearanceCopperNpth->getValue());
    s.setMinDrillDrillClearance(mUi->edtDrcClearanceDrillDrill->getValue());
    s.setMinDrillBoardClearance(mUi->edtDrcClearanceDrillBoard->getValue());
    s.setMinSilkscreenStopmaskClearance(
        mUi->edtDrcClearanceSilkscreenStopmask->getValue());
    s.setMinCopperWidth(mUi->edtDrcMinCopperWidth->getValue());
    s.setMinPthAnnularRing(mUi->edtDrcMinPthAnnularRing->getValue());
    s.setMinNpthDrillDiameter(mUi->edtDrcMinNpthDrillDiameter->getValue());
    s.setMinNpthSlotWidth(mUi->edtDrcMinNpthSlotWidth->getValue());
    s.setMinPthDrillDiameter(mUi->edtDrcMinPthDrillDiameter->getValue());
    s.setMinPthSlotWidth(mUi->edtDrcMinPthSlotWidth->getValue());
    s.setMinSilkscreenWidth(mUi->edtDrcMinSilkscreenWidth->getValue());
    s.setMinSilkscreenTextHeight(
        mUi->edtDrcMinSilkscreenTextHeight->getValue());
    s.setMinOutlineToolDiameter(mUi->edtDrcMinOutlineToolDiameter->getValue());
    s.setBlindViasAllowed(mUi->cbxBlindViasAllowed->isChecked());
    s.setBuriedViasAllowed(mUi->cbxBuriedViasAllowed->isChecked());
    s.setAllowedNpthSlots(
        mUi->cbxDrcAllowedNpthSlots->currentData()
            .value<BoardDesignRuleCheckSettings::AllowedSlots>());
    s.setAllowedPthSlots(
        mUi->cbxDrcAllowedPthSlots->currentData()
            .value<BoardDesignRuleCheckSettings::AllowedSlots>());
    cmd->setDrcSettings(s);

    mUndoStack.execCmd(cmd.take());  // can throw
    return true;
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Could not apply settings"), e.getMsg());
    return false;
  }
}

QVector<const Layer*> BoardSetupDialog::getTopSilkscreenLayers()
    const noexcept {
  QVector<const Layer*> layers;
  if (mUi->cbxSilkTopLegend->isChecked()) {
    layers << &Layer::topLegend();
  }
  if (mUi->cbxSilkTopNames->isChecked()) {
    layers << &Layer::topNames();
  }
  if (mUi->cbxSilkTopValues->isChecked()) {
    layers << &Layer::topValues();
  }
  return layers;
}

QVector<const Layer*> BoardSetupDialog::getBotSilkscreenLayers()
    const noexcept {
  QVector<const Layer*> layers;
  if (mUi->cbxSilkBotLegend->isChecked()) {
    layers << &Layer::botLegend();
  }
  if (mUi->cbxSilkBotNames->isChecked()) {
    layers << &Layer::botNames();
  }
  if (mUi->cbxSilkBotValues->isChecked()) {
    layers << &Layer::botValues();
  }
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
