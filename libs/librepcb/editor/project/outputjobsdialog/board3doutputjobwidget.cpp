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
#include "board3doutputjobwidget.h"

#include "ui_board3doutputjobwidget.h"

#include <librepcb/core/job/board3doutputjob.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
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

Board3DOutputJobWidget::Board3DOutputJobWidget(
    Project& project, std::shared_ptr<Board3DOutputJob> job,
    QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mJob(job),
    mUi(new Ui::Board3DOutputJobWidget) {
  mUi->setupUi(this);

  // Name.
  mUi->edtName->setText(*job->getName());
  connect(mUi->edtName, &QLineEdit::textEdited, this, [this](QString text) {
    text = cleanElementName(text);
    if (!text.isEmpty()) {
      mJob->setName(ElementName(text));
    }
  });

  // Output path.
  mUi->edtOutput->setText(job->getOutputPath());
  connect(mUi->edtOutput, &QLineEdit::textEdited, this, [this](QString text) {
    mJob->setOutputPath(text.replace("\\", "/").trimmed());
  });

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
          &Board3DOutputJobWidget::applyBoards);

  // Boards.
  connect(mUi->rbtnBoardsAll, &QRadioButton::toggled, this,
          &Board3DOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsDefault, &QRadioButton::toggled, this,
          &Board3DOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsCustom, &QRadioButton::toggled, this,
          &Board3DOutputJobWidget::applyBoards);
  mUi->rbtnBoardsAll->setChecked(job->getBoards().isAll());
  mUi->rbtnBoardsDefault->setChecked(job->getBoards().isDefault());
  mUi->rbtnBoardsCustom->setChecked(job->getBoards().isCustom());

  // List custom assembly variants.
  QVector<tl::optional<Uuid>> allVariantUuids;
  QHash<Uuid, QString> avNames;
  allVariantUuids.append(tl::nullopt);
  for (const auto& av : mProject.getCircuit().getAssemblyVariants()) {
    allVariantUuids.append(av.getUuid());
    avNames[av.getUuid()] = av.getDisplayText();
  }
  foreach (const tl::optional<Uuid>& uuid,
           mJob->getAssemblyVariants().getSet()) {
    if (!allVariantUuids.contains(uuid)) {
      allVariantUuids.append(uuid);
    }
  }
  foreach (const tl::optional<Uuid>& uuid, allVariantUuids) {
    QListWidgetItem* item =
        new QListWidgetItem(uuid ? avNames.value(*uuid, uuid->toStr())
                                 : tr("None (just the plain PCB)"),
                            mUi->lstVariants);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    item->setCheckState(mJob->getAssemblyVariants().getSet().contains(uuid)
                            ? Qt::Checked
                            : Qt::Unchecked);
    item->setData(Qt::UserRole, uuid ? uuid->toStr() : QString());
  }
  connect(mUi->lstVariants, &QListWidget::itemChanged, this,
          &Board3DOutputJobWidget::applyVariants);

  // Assembly variants.
  connect(mUi->rbtnVariantsAll, &QRadioButton::toggled, this,
          &Board3DOutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsDefault, &QRadioButton::toggled, this,
          &Board3DOutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsCustom, &QRadioButton::toggled, this,
          &Board3DOutputJobWidget::applyVariants);
  mUi->rbtnVariantsAll->setChecked(job->getAssemblyVariants().isAll());
  mUi->rbtnVariantsDefault->setChecked(job->getAssemblyVariants().isDefault());
  mUi->rbtnVariantsCustom->setChecked(job->getAssemblyVariants().isCustom());
}

Board3DOutputJobWidget::~Board3DOutputJobWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Board3DOutputJobWidget::applyBoards(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnBoardsAll->isChecked()) {
    mJob->setBoards(Board3DOutputJob::BoardSet::all());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsDefault->isChecked()) {
    mJob->setBoards(Board3DOutputJob::BoardSet::onlyDefault());
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
    mJob->setBoards(Board3DOutputJob::BoardSet::set(uuids));
    mUi->lstBoards->setEnabled(true);
  }
}

void Board3DOutputJobWidget::applyVariants(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnVariantsAll->isChecked()) {
    mJob->setAssemblyVariants(Board3DOutputJob::AssemblyVariantSet::all());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsDefault->isChecked()) {
    mJob->setAssemblyVariants(
        Board3DOutputJob::AssemblyVariantSet::onlyDefault());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsCustom->isChecked()) {
    QSet<tl::optional<Uuid>> uuids;
    for (int i = 0; i < mUi->lstVariants->count(); ++i) {
      if (QListWidgetItem* item = mUi->lstVariants->item(i)) {
        auto uuid = Uuid::tryFromString(item->data(Qt::UserRole).toString());
        if (item->checkState() == Qt::Checked) {
          uuids.insert(uuid);
        }
      }
    }
    mJob->setAssemblyVariants(Board3DOutputJob::AssemblyVariantSet::set(uuids));
    mUi->lstVariants->setEnabled(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
