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
#include "pickplaceoutputjobwidget.h"

#include "ui_pickplaceoutputjobwidget.h"

#include <librepcb/core/job/pickplaceoutputjob.h>
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

PickPlaceOutputJobWidget::PickPlaceOutputJobWidget(
    Project& project, std::shared_ptr<PickPlaceOutputJob> job,
    QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mJob(job),
    mUi(new Ui::PickPlaceOutputJobWidget) {
  mUi->setupUi(this);

  // Name.
  mUi->edtName->setText(*job->getName());
  connect(mUi->edtName, &QLineEdit::textEdited, this, [this](QString text) {
    text = cleanElementName(text);
    if (!text.isEmpty()) {
      mJob->setName(ElementName(text));
    }
  });

  // Technologies.
  auto setupTechnology = [this](QCheckBox* checkBox,
                                PickPlaceOutputJob::Technology flag) {
    checkBox->setChecked(mJob->getTechnologies().testFlag(flag));
    connect(checkBox, &QCheckBox::toggled, this, [this, flag](bool checked) {
      auto tmp = mJob->getTechnologies();
      tmp.setFlag(flag, checked);
      mJob->setTechnologies(tmp);
    });
  };
  setupTechnology(mUi->cbxTechnologyTht, PickPlaceOutputJob::Technology::Tht);
  setupTechnology(mUi->cbxTechnologySmt, PickPlaceOutputJob::Technology::Smt);
  setupTechnology(mUi->cbxTechnologyMixed,
                  PickPlaceOutputJob::Technology::Mixed);
  setupTechnology(mUi->cbxTechnologyFiducial,
                  PickPlaceOutputJob::Technology::Fiducial);
  setupTechnology(mUi->cbxTechnologyOther,
                  PickPlaceOutputJob::Technology::Other);

  // Output path.
  auto setupOutputPath =
      [this](QCheckBox* checkBox, QLineEdit* lineEdit,
             bool (PickPlaceOutputJob::*getCreate)() const noexcept,
             void (PickPlaceOutputJob::*setCreate)(bool) noexcept,
             const QString& (PickPlaceOutputJob::*getPath)() const noexcept,
             void (PickPlaceOutputJob::*setPath)(const QString&) noexcept) {
        connect(checkBox, &QCheckBox::toggled, lineEdit,
                &QLineEdit::setEnabled);
        checkBox->setChecked((*mJob.*getCreate)());
        connect(
            checkBox, &QCheckBox::toggled, this,
            [this, setCreate](bool checked) { (*mJob.*setCreate)(checked); });
        lineEdit->setText((*mJob.*getPath)());
        connect(lineEdit, &QLineEdit::textEdited, this,
                [this, setPath](QString text) {
                  (*mJob.*setPath)(text.replace("\\", "/").trimmed());
                });
      };
  setupOutputPath(mUi->cbxCreateTop, mUi->edtOutputTop,  // break
                  &PickPlaceOutputJob::getCreateTop,  // break
                  &PickPlaceOutputJob::setCreateTop,  // break
                  &PickPlaceOutputJob::getOutputPathTop,  // break
                  &PickPlaceOutputJob::setOutputPathTop);
  setupOutputPath(mUi->cbxCreateBottom, mUi->edtOutputBottom,  // break
                  &PickPlaceOutputJob::getCreateBottom,  // break
                  &PickPlaceOutputJob::setCreateBottom,  // break
                  &PickPlaceOutputJob::getOutputPathBottom,  // break
                  &PickPlaceOutputJob::setOutputPathBottom);
  setupOutputPath(mUi->cbxCreateBoth, mUi->edtOutputBoth,  // break
                  &PickPlaceOutputJob::getCreateBoth,  // break
                  &PickPlaceOutputJob::setCreateBoth,  // break
                  &PickPlaceOutputJob::getOutputPathBoth,  // break
                  &PickPlaceOutputJob::setOutputPathBoth);

  // Include comments.
  mUi->cbxIncludeComment->setChecked(mJob->getIncludeComment());
  connect(mUi->cbxIncludeComment, &QCheckBox::toggled, this,
          [this](bool checked) { mJob->setIncludeComment(checked); });

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
          &PickPlaceOutputJobWidget::applyBoards);

  // Boards.
  connect(mUi->rbtnBoardsAll, &QRadioButton::toggled, this,
          &PickPlaceOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsDefault, &QRadioButton::toggled, this,
          &PickPlaceOutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsCustom, &QRadioButton::toggled, this,
          &PickPlaceOutputJobWidget::applyBoards);
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
          &PickPlaceOutputJobWidget::applyVariants);

  // Assembly variants.
  connect(mUi->rbtnVariantsAll, &QRadioButton::toggled, this,
          &PickPlaceOutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsDefault, &QRadioButton::toggled, this,
          &PickPlaceOutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsCustom, &QRadioButton::toggled, this,
          &PickPlaceOutputJobWidget::applyVariants);
  mUi->rbtnVariantsAll->setChecked(job->getAssemblyVariants().isAll());
  mUi->rbtnVariantsDefault->setChecked(job->getAssemblyVariants().isDefault());
  mUi->rbtnVariantsCustom->setChecked(job->getAssemblyVariants().isCustom());
}

PickPlaceOutputJobWidget::~PickPlaceOutputJobWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PickPlaceOutputJobWidget::applyBoards(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnBoardsAll->isChecked()) {
    mJob->setBoards(PickPlaceOutputJob::BoardSet::all());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsDefault->isChecked()) {
    mJob->setBoards(PickPlaceOutputJob::BoardSet::onlyDefault());
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
    mJob->setBoards(PickPlaceOutputJob::BoardSet::set(uuids));
    mUi->lstBoards->setEnabled(true);
  }
}

void PickPlaceOutputJobWidget::applyVariants(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnVariantsAll->isChecked()) {
    mJob->setAssemblyVariants(PickPlaceOutputJob::AssemblyVariantSet::all());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsDefault->isChecked()) {
    mJob->setAssemblyVariants(
        PickPlaceOutputJob::AssemblyVariantSet::onlyDefault());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsCustom->isChecked()) {
    QSet<Uuid> uuids;
    for (int i = 0; i < mUi->lstVariants->count(); ++i) {
      if (QListWidgetItem* item = mUi->lstVariants->item(i)) {
        auto uuid = Uuid::tryFromString(item->data(Qt::UserRole).toString());
        if ((item->checkState() == Qt::Checked) && uuid) {
          uuids.insert(*uuid);
        }
      }
    }
    mJob->setAssemblyVariants(
        PickPlaceOutputJob::AssemblyVariantSet::set(uuids));
    mUi->lstVariants->setEnabled(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
