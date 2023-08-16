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
#include "gerberx3outputjobwidget.h"

#include "ui_gerberx3outputjobwidget.h"

#include <librepcb/core/job/gerberx3outputjob.h>
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

GerberX3OutputJobWidget::GerberX3OutputJobWidget(
    Project& project, std::shared_ptr<GerberX3OutputJob> job,
    QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mJob(job),
    mUi(new Ui::GerberX3OutputJobWidget) {
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
  auto setupOutputPath =
      [this](QCheckBox* checkBox, QLineEdit* lineEdit,
             bool (GerberX3OutputJob::*getCreate)() const noexcept,
             void (GerberX3OutputJob::*setCreate)(bool) noexcept,
             const QString& (GerberX3OutputJob::*getPath)() const noexcept,
             void (GerberX3OutputJob::*setPath)(const QString&) noexcept) {
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
                  &GerberX3OutputJob::getCreateTop,  // break
                  &GerberX3OutputJob::setCreateTop,  // break
                  &GerberX3OutputJob::getOutputPathTop,  // break
                  &GerberX3OutputJob::setOutputPathTop);
  setupOutputPath(mUi->cbxCreateBottom, mUi->edtOutputBottom,  // break
                  &GerberX3OutputJob::getCreateBottom,  // break
                  &GerberX3OutputJob::setCreateBottom,  // break
                  &GerberX3OutputJob::getOutputPathBottom,  // break
                  &GerberX3OutputJob::setOutputPathBottom);

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
          &GerberX3OutputJobWidget::applyBoards);

  // Boards.
  connect(mUi->rbtnBoardsAll, &QRadioButton::toggled, this,
          &GerberX3OutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsDefault, &QRadioButton::toggled, this,
          &GerberX3OutputJobWidget::applyBoards);
  connect(mUi->rbtnBoardsCustom, &QRadioButton::toggled, this,
          &GerberX3OutputJobWidget::applyBoards);
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
          &GerberX3OutputJobWidget::applyVariants);

  // Assembly variants.
  connect(mUi->rbtnVariantsAll, &QRadioButton::toggled, this,
          &GerberX3OutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsDefault, &QRadioButton::toggled, this,
          &GerberX3OutputJobWidget::applyVariants);
  connect(mUi->rbtnVariantsCustom, &QRadioButton::toggled, this,
          &GerberX3OutputJobWidget::applyVariants);
  mUi->rbtnVariantsAll->setChecked(job->getAssemblyVariants().isAll());
  mUi->rbtnVariantsDefault->setChecked(job->getAssemblyVariants().isDefault());
  mUi->rbtnVariantsCustom->setChecked(job->getAssemblyVariants().isCustom());
}

GerberX3OutputJobWidget::~GerberX3OutputJobWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GerberX3OutputJobWidget::applyBoards(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnBoardsAll->isChecked()) {
    mJob->setBoards(GerberX3OutputJob::BoardSet::all());
    mUi->lstBoards->setEnabled(false);
  } else if (mUi->rbtnBoardsDefault->isChecked()) {
    mJob->setBoards(GerberX3OutputJob::BoardSet::onlyDefault());
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
    mJob->setBoards(GerberX3OutputJob::BoardSet::set(uuids));
    mUi->lstBoards->setEnabled(true);
  }
}

void GerberX3OutputJobWidget::applyVariants(bool checked) noexcept {
  if (!checked) {
    return;
  }

  if (mUi->rbtnVariantsAll->isChecked()) {
    mJob->setAssemblyVariants(GerberX3OutputJob::AssemblyVariantSet::all());
    mUi->lstVariants->setEnabled(false);
  } else if (mUi->rbtnVariantsDefault->isChecked()) {
    mJob->setAssemblyVariants(
        GerberX3OutputJob::AssemblyVariantSet::onlyDefault());
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
        GerberX3OutputJob::AssemblyVariantSet::set(uuids));
    mUi->lstVariants->setEnabled(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
