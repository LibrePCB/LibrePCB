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
#include "outputjoblistwidgetitem.h"

#include "ui_outputjoblistwidgetitem.h"

#include <librepcb/core/job/outputjob.h>

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

OutputJobListWidgetItem::OutputJobListWidgetItem(std::shared_ptr<OutputJob> job,
                                                 QWidget* parent) noexcept
  : QFrame(parent), mJob(job), mUi(new Ui::OutputJobListWidgetItem) {
  mUi->setupUi(this);
  setObjectName("OutputJobListWidgetItem");  // For the stylesheet below.
  if (job) {
    mUi->btnOpenDirectory->hide();
    mUi->lblType->setText(job->getTypeTr());
    setStatusColor(Qt::transparent);
  } else {
    QFont font = mUi->lblName->font();
    font.setBold(true);
    mUi->lblName->setFont(font);
    mUi->lblName->setText(tr("Output Jobs"));
    mUi->lblIcon->setPixmap(
        QIcon(":/img/actions/output_jobs.png").pixmap(mUi->lblIcon->size()));
    mUi->btnRun->setFixedSize(mUi->btnOpenDirectory->size());
    mUi->btnRun->setIconSize(mUi->btnOpenDirectory->iconSize());
    mUi->lblType->hide();
    setStyleSheet(
        "#OutputJobListWidgetItem{"
        "  border-top-style: none;"
        "  border-left-style: none;"
        "  border-right-style: none;"
        "  border-bottom: 1px solid gray;"
        "}");
    setFrameShape(Box);
  }
  updateJobInfo();
  connect(mUi->btnOpenDirectory, &QToolButton::clicked, this,
          &OutputJobListWidgetItem::openDirectoryTriggered);
  connect(mUi->btnRun, &QToolButton::clicked, this,
          [this]() { emit runTriggered(mJob); });
}

OutputJobListWidgetItem::~OutputJobListWidgetItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString OutputJobListWidgetItem::getTitle() const noexcept {
  return mUi->lblName->text();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OutputJobListWidgetItem::setSelected(bool selected) noexcept {
  QString stylesheet;
  if (selected) {
    stylesheet = "color: palette(highlighted-text);";
  }
  mUi->lblName->setStyleSheet(stylesheet);
  mUi->lblType->setStyleSheet(stylesheet);
}

void OutputJobListWidgetItem::updateJobInfo() noexcept {
  if (mJob) {
    mUi->lblIcon->setPixmap(mJob->getTypeIcon().pixmap(mUi->lblIcon->size()));
    mUi->lblName->setText(*mJob->getName());
    mUi->btnRun->setToolTip(mJob->getDependencies().isEmpty()
                                ? tr("Run this job")
                                : tr("Run all dependencies and this job"));
  } else {
    mUi->btnRun->setToolTip(tr("Run all jobs"));
  }
  mUi->btnRun->setIcon((mJob && mJob->getDependencies().isEmpty())
                           ? QIcon(":/img/actions/run.png")
                           : QIcon(":/img/actions/run_all.png"));
}

void OutputJobListWidgetItem::setStatusColor(const QColor& color) noexcept {
  if (mJob) {
    setStyleSheet(QString("#OutputJobListWidgetItem{"
                          "  border-top-style: none;"
                          "  border-left-style: none;"
                          "  border-right: 3px solid %1;"
                          "  border-bottom-style: none;"
                          "}")
                      .arg(color.name(QColor::HexArgb)));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
