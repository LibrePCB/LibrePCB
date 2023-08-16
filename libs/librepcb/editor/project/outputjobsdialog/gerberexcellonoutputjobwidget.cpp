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
#include "gerberexcellonoutputjobwidget.h"

#include "ui_gerberexcellonoutputjobwidget.h"

#include <librepcb/core/job/gerberexcellonoutputjob.h>
#include <librepcb/core/project/board/board.h>
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

GerberExcellonOutputJobWidget::GerberExcellonOutputJobWidget(
    Project& project, std::shared_ptr<GerberExcellonOutputJob> job,
    QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mJob(job),
    mUi(new Ui::GerberExcellonOutputJobWidget) {
  mUi->setupUi(this);

  // Info.
  QString infos;
  infos += "<p><b>" %
      tr("Note that it's highly recommended to review the generated files "
         "before ordering PCBs.") %
      "</b><br>";
  infos += tr("This could be done with the free application <a "
              "href=\"%1\">gerbv</a> or the <a href=\"%2\">official reference "
              "viewer from Ucamco</a>.")
               .arg("http://gerbv.geda-project.org/")
               .arg("https://gerber.ucamco.com/") %
      "</p>";
  infos += "<p>" %
      tr("As a simpler and faster alternative, you could use the "
         "<a href=\"%1\">Order PCB</a> feature instead.")
          .arg("order-pcb") %
      "</p>";
  mUi->lblInfo->setText(infos);
  connect(mUi->lblInfo, &QLabel::linkActivated, this,
          [this](const QString& link) {
            if (link == "order-pcb") {
              emit orderPcbDialogTriggered();
            } else {
              emit openUrlRequested(QUrl(link));
            }
          });

  // Name.
  mUi->edtName->setText(*job->getName());
  connect(mUi->edtName, &QLineEdit::textEdited, this, [this](QString text) {
    text = cleanElementName(text);
    if (!text.isEmpty()) {
      mJob->setName(ElementName(text));
    }
  });

  // Base path.
  mUi->edtBasePath->setText(job->getOutputPath());
  connect(mUi->edtBasePath, &QLineEdit::textEdited, this, [this](QString text) {
    mJob->setOutputPath(text.replace("\\", "/").trimmed());
  });

  // Suffixes.
  mUi->edtSuffixOutlines->setText(mJob->getSuffixOutlines());
  connect(mUi->edtSuffixOutlines, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixOutlines(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixCopperTop->setText(mJob->getSuffixCopperTop());
  connect(mUi->edtSuffixCopperTop, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixCopperTop(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixCopperInner->setText(mJob->getSuffixCopperInner());
  connect(mUi->edtSuffixCopperInner, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixCopperInner(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixCopperBot->setText(mJob->getSuffixCopperBot());
  connect(mUi->edtSuffixCopperBot, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixCopperBot(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixSoldermaskTop->setText(mJob->getSuffixSolderMaskTop());
  connect(mUi->edtSuffixSoldermaskTop, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixSolderMaskTop(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixSoldermaskBot->setText(mJob->getSuffixSolderMaskBot());
  connect(mUi->edtSuffixSoldermaskBot, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixSolderMaskBot(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixSilkscreenTop->setText(mJob->getSuffixSilkscreenTop());
  connect(mUi->edtSuffixSilkscreenTop, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixSilkscreenTop(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixSilkscreenBot->setText(mJob->getSuffixSilkscreenBot());
  connect(mUi->edtSuffixSilkscreenBot, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixSilkscreenBot(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixDrillsNpth->setText(mJob->getSuffixDrillsNpth());
  connect(mUi->edtSuffixDrillsNpth, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixDrillsNpth(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixDrillsPth->setText(mJob->getSuffixDrillsPth());
  connect(mUi->edtSuffixDrillsPth, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixDrillsPth(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixDrills->setText(mJob->getSuffixDrills());
  connect(mUi->edtSuffixDrills, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixDrills(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixDrillsBuried->setText(mJob->getSuffixDrillsBlindBuried());
  connect(mUi->edtSuffixDrillsBuried, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixDrillsBlindBuried(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixSolderPasteTop->setText(mJob->getSuffixSolderPasteTop());
  connect(mUi->edtSuffixSolderPasteTop, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixSolderPasteTop(text.replace("\\", "/").trimmed());
          });
  mUi->edtSuffixSolderPasteBot->setText(mJob->getSuffixSolderPasteBot());
  connect(mUi->edtSuffixSolderPasteBot, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setSuffixSolderPasteBot(text.replace("\\", "/").trimmed());
          });

  // Checkboxes.
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, mUi->edtSuffixDrills,
          &QLineEdit::setEnabled);
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, mUi->edtSuffixDrillsNpth,
          &QLineEdit::setDisabled);
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, mUi->edtSuffixDrillsPth,
          &QLineEdit::setDisabled);
  mUi->cbxDrillsMerge->setChecked(mJob->getMergeDrillFiles());
  connect(mUi->cbxDrillsMerge, &QCheckBox::toggled, this,
          [this](bool checked) { mJob->setMergeDrillFiles(checked); });
  mUi->cbxUseG85Slots->setChecked(mJob->getUseG85SlotCommand());
  connect(mUi->cbxUseG85Slots, &QCheckBox::toggled, this,
          [this](bool checked) { mJob->setUseG85SlotCommand(checked); });
  connect(mUi->cbxSolderPasteTop, &QCheckBox::toggled,
          mUi->edtSuffixSolderPasteTop, &QLineEdit::setEnabled);
  mUi->cbxSolderPasteTop->setChecked(mJob->getEnableSolderPasteTop());
  connect(mUi->cbxSolderPasteTop, &QCheckBox::toggled, this,
          [this](bool checked) { mJob->setEnableSolderPasteTop(checked); });
  connect(mUi->cbxSolderPasteBot, &QCheckBox::toggled,
          mUi->edtSuffixSolderPasteBot, &QLineEdit::setEnabled);
  mUi->cbxSolderPasteBot->setChecked(mJob->getEnableSolderPasteBot());
  connect(mUi->cbxSolderPasteBot, &QCheckBox::toggled, this,
          [this](bool checked) { mJob->setEnableSolderPasteBot(checked); });

  // List custom boards.
  QList<Uuid> allBoardUuids;
  QHash<Uuid, QString> boardNames;
  foreach (const Board* board, mProject.getBoards()) {
    allBoardUuids.append(board->getUuid());
    boardNames[board->getUuid()] = *board->getName();
  }
  foreach (const Uuid& uuid, mJob->getBoards().getSet()) {
    if (!allBoardUuids.contains(uuid)) {
      allBoardUuids.append(uuid);
    }
  }
  foreach (const Uuid& uuid, allBoardUuids) {
    QListWidgetItem* item = new QListWidgetItem(
        boardNames.value(uuid, uuid.toStr()), mUi->lstBoards);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    item->setCheckState(mJob->getBoards().getSet().contains(uuid)
                            ? Qt::Checked
                            : Qt::Unchecked);
    item->setData(Qt::UserRole, uuid.toStr());
  }
  connect(mUi->lstBoards, &QListWidget::itemChanged, this,
          &GerberExcellonOutputJobWidget::applyBoards);

  // Boards.
  connect(mUi->rbtnBoardsAll, &QRadioButton::toggled, this,
          &GerberExcellonOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsDefault, &QRadioButton::toggled, this,
          &GerberExcellonOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsCustom, &QRadioButton::toggled, this,
          &GerberExcellonOutputJobWidget::applyBoards);
  mUi->rbtnBoardsAll->setChecked(job->getBoards().isAll());
  mUi->rbtnBoardsDefault->setChecked(job->getBoards().isDefault());
  mUi->rbtnBoardsCustom->setChecked(job->getBoards().isCustom());
}

GerberExcellonOutputJobWidget::~GerberExcellonOutputJobWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GerberExcellonOutputJobWidget::applyBoards(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnBoardsAll->isChecked()) {
    mJob->setBoards(GerberExcellonOutputJob::BoardSet::all());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsDefault->isChecked()) {
    mJob->setBoards(GerberExcellonOutputJob::BoardSet::onlyDefault());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsCustom->isChecked()) {
    QSet<Uuid> uuids;
    for (int i = 0; i < mUi->lstBoards->count(); ++i) {
      if (QListWidgetItem* item = mUi->lstBoards->item(i)) {
        auto uuid = Uuid::tryFromString(item->data(Qt::UserRole).toString());
        if ((item->checkState() == Qt::Checked) && uuid) {
          uuids.insert(*uuid);
        }
      }
    }
    mJob->setBoards(GerberExcellonOutputJob::BoardSet::set(uuids));
    mUi->lstBoards->setEnabled(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
