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
#include "bomoutputjobwidget.h"

#include "ui_bomoutputjobwidget.h"

#include <librepcb/core/job/bomoutputjob.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/qtcompat.h>

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

BomOutputJobWidget::BomOutputJobWidget(Project& project,
                                       std::shared_ptr<BomOutputJob> job,
                                       QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mJob(job),
    mUi(new Ui::BomOutputJobWidget) {
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

  // Custom attributes.
  mUi->edtCustomAttributes->setText(job->getCustomAttributes().join(", "));
  connect(mUi->edtCustomAttributes, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setCustomAttributes(
                text.remove(" ").split(",", QtCompat::skipEmptyParts()));
          });

  // List custom boards.
  QVector<tl::optional<Uuid>> allBoardUuids;
  QHash<Uuid, QString> boardNames;
  allBoardUuids.append(tl::nullopt);
  foreach (const Board* board, mProject.getBoards()) {
    allBoardUuids.append(board->getUuid());
    boardNames[board->getUuid()] = *board->getName();
  }
  foreach (const tl::optional<Uuid>& uuid, mJob->getBoards().getSet()) {
    if (!allBoardUuids.contains(uuid)) {
      allBoardUuids.append(uuid);
    }
  }
  foreach (const tl::optional<Uuid>& uuid, allBoardUuids) {
    QListWidgetItem* item = new QListWidgetItem(
        uuid ? boardNames.value(*uuid, uuid->toStr()) : tr("None (generic)"),
        mUi->lstBoards);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    item->setCheckState(mJob->getBoards().getSet().contains(uuid)
                            ? Qt::Checked
                            : Qt::Unchecked);
    item->setData(Qt::UserRole, uuid ? uuid->toStr() : QString());
  }
  connect(mUi->lstBoards, &QListWidget::itemChanged, this,
          &BomOutputJobWidget::applyBoards);

  // Boards.
  connect(mUi->rbtnBoardsAll, &QRadioButton::toggled, this,
          &BomOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsDefault, &QRadioButton::toggled, this,
          &BomOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsCustom, &QRadioButton::toggled, this,
          &BomOutputJobWidget::applyBoards);
  mUi->rbtnBoardsAll->setChecked(job->getBoards().isAll());
  mUi->rbtnBoardsDefault->setChecked(job->getBoards().isDefault());
  mUi->rbtnBoardsCustom->setChecked(job->getBoards().isCustom());

  // List custom assembly variants.
  QList<Uuid> allVariantUuids;
  QHash<Uuid, QString> avNames;
  for (const auto& av : mProject.getCircuit().getAssemblyVariants()) {
    allVariantUuids.append(av.getUuid());
    avNames[av.getUuid()] = av.getDisplayText();
  }
  foreach (const Uuid& uuid, mJob->getAssemblyVariants().getSet()) {
    if (!allVariantUuids.contains(uuid)) {
      allVariantUuids.append(uuid);
    }
  }
  foreach (const Uuid& uuid, allVariantUuids) {
    QListWidgetItem* item = new QListWidgetItem(
        avNames.value(uuid, uuid.toStr()), mUi->lstVariants);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    item->setCheckState(mJob->getAssemblyVariants().getSet().contains(uuid)
                            ? Qt::Checked
                            : Qt::Unchecked);
    item->setData(Qt::UserRole, uuid.toStr());
  }
  connect(mUi->lstVariants, &QListWidget::itemChanged, this,
          &BomOutputJobWidget::applyVariants);

  // Assembly variants.
  connect(mUi->rbtnVariantsAll, &QRadioButton::toggled, this,
          &BomOutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsDefault, &QRadioButton::toggled, this,
          &BomOutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsCustom, &QRadioButton::toggled, this,
          &BomOutputJobWidget::applyVariants);
  mUi->rbtnVariantsAll->setChecked(job->getAssemblyVariants().isAll());
  mUi->rbtnVariantsDefault->setChecked(job->getAssemblyVariants().isDefault());
  mUi->rbtnVariantsCustom->setChecked(job->getAssemblyVariants().isCustom());
}

BomOutputJobWidget::~BomOutputJobWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BomOutputJobWidget::applyBoards(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnBoardsAll->isChecked()) {
    mJob->setBoards(BomOutputJob::BoardSet::all());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsDefault->isChecked()) {
    mJob->setBoards(BomOutputJob::BoardSet::onlyDefault());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsCustom->isChecked()) {
    QSet<tl::optional<Uuid>> uuids;
    for (int i = 0; i < mUi->lstBoards->count(); ++i) {
      if (QListWidgetItem* item = mUi->lstBoards->item(i)) {
        auto uuid = Uuid::tryFromString(item->data(Qt::UserRole).toString());
        if (item->checkState() == Qt::Checked) {
          uuids.insert(uuid);
        }
      }
    }
    mJob->setBoards(BomOutputJob::BoardSet::set(uuids));
    mUi->lstBoards->setEnabled(true);
  }
}

void BomOutputJobWidget::applyVariants(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnVariantsAll->isChecked()) {
    mJob->setAssemblyVariants(BomOutputJob::AssemblyVariantSet::all());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsDefault->isChecked()) {
    mJob->setAssemblyVariants(BomOutputJob::AssemblyVariantSet::onlyDefault());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsCustom->isChecked()) {
    QSet<Uuid> uuids;
    for (int i = 0; i < mUi->lstVariants->count(); ++i) {
      if (QListWidgetItem* item = mUi->lstVariants->item(i)) {
        auto uuid = Uuid::tryFromString(item->data(Qt::UserRole).toString());
        if ((item->checkState() == Qt::Checked) && (uuid)) {
          uuids.insert(*uuid);
        }
      }
    }
    mJob->setAssemblyVariants(BomOutputJob::AssemblyVariantSet::set(uuids));
    mUi->lstVariants->setEnabled(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
