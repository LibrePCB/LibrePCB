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
#include "archiveoutputjobwidget.h"

#include "ui_archiveoutputjobwidget.h"

#include <librepcb/core/job/archiveoutputjob.h>
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

ArchiveOutputJobWidget::ArchiveOutputJobWidget(
    Project& project, const OutputJobList& allJobs,
    std::shared_ptr<ArchiveOutputJob> job, QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mAllJobs(allJobs),
    mJob(job),
    mUi(new Ui::ArchiveOutputJobWidget) {
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

  // Input jobs.
  mUi->tblInput->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  mUi->tblInput->horizontalHeader()->setSectionResizeMode(1,
                                                          QHeaderView::Stretch);
  mUi->tblInput->horizontalHeader()->setSectionResizeMode(2,
                                                          QHeaderView::Stretch);
  QList<Uuid> allUuids;
  QHash<Uuid, QString> jobNames;
  QHash<Uuid, QString> outputDirs;
  for (const auto& item : mAllJobs) {
    if (item.getUuid() != mJob->getUuid()) {
      allUuids.append(item.getUuid());
      jobNames[item.getUuid()] = *item.getName();
    }
  }
  for (auto it = mJob->getInputJobs().begin(); it != mJob->getInputJobs().end();
       ++it) {
    if (!allUuids.contains(it.key())) {
      allUuids.append(it.key());
    }
    outputDirs[it.key()] = it.value();
  }
  mUi->tblInput->setRowCount(allUuids.count());
  for (int i = 0; i < allUuids.count(); ++i) {
    const bool checked = outputDirs.contains(allUuids.at(i));
    QTableWidgetItem* cbxItem = new QTableWidgetItem();
    cbxItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    cbxItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    mUi->tblInput->setItem(i, 0, cbxItem);
    QTableWidgetItem* nameItem = new QTableWidgetItem();
    nameItem->setText(jobNames.value(allUuids.at(i), allUuids.at(i).toStr()));
    nameItem->setData(Qt::UserRole, allUuids.at(i).toStr());
    mUi->tblInput->setItem(i, 1, nameItem);
    QTableWidgetItem* dirItem = new QTableWidgetItem();
    dirItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
    dirItem->setText(outputDirs.value(allUuids.at(i)));
    mUi->tblInput->setItem(i, 2, dirItem);
  }
  connect(mUi->tblInput, &QTableWidget::itemChanged, this,
          &ArchiveOutputJobWidget::applyInputJobs);
}

ArchiveOutputJobWidget::~ArchiveOutputJobWidget() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

void ArchiveOutputJobWidget::applyInputJobs() noexcept {
  QMap<Uuid, QString> input;
  for (int i = 0; i < mUi->tblInput->rowCount(); ++i) {
    const QTableWidgetItem* cbxItem = mUi->tblInput->item(i, 0);
    const QTableWidgetItem* nameItem = mUi->tblInput->item(i, 1);
    const QTableWidgetItem* dirItem = mUi->tblInput->item(i, 2);
    Q_ASSERT(cbxItem && nameItem && dirItem);
    auto uuid = Uuid::tryFromString(nameItem->data(Qt::UserRole).toString());
    if ((cbxItem->checkState() == Qt::Checked) && uuid) {
      input[*uuid] = dirItem->text().replace("\\", "/").trimmed();
    }
  }
  mJob->setInputJobs(input);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
