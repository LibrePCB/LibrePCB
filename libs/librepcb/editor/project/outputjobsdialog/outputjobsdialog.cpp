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
#include "outputjobsdialog.h"

#include "../../editorcommandset.h"
#include "../../undostack.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdprojectedit.h"
#include "archiveoutputjobwidget.h"
#include "board3doutputjobwidget.h"
#include "bomoutputjobwidget.h"
#include "copyoutputjobwidget.h"
#include "gerberexcellonoutputjobwidget.h"
#include "gerberx3outputjobwidget.h"
#include "graphicsoutputjobwidget.h"
#include "lppzoutputjobwidget.h"
#include "netlistoutputjobwidget.h"
#include "outputjobhomewidget.h"
#include "outputjoblistwidgetitem.h"
#include "pickplaceoutputjobwidget.h"
#include "projectjsonoutputjobwidget.h"
#include "ui_outputjobsdialog.h"

#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/outputdirectorywriter.h>
#include <librepcb/core/job/archiveoutputjob.h>
#include <librepcb/core/job/board3doutputjob.h>
#include <librepcb/core/job/bomoutputjob.h>
#include <librepcb/core/job/copyoutputjob.h>
#include <librepcb/core/job/gerberexcellonoutputjob.h>
#include <librepcb/core/job/gerberx3outputjob.h>
#include <librepcb/core/job/graphicsoutputjob.h>
#include <librepcb/core/job/lppzoutputjob.h>
#include <librepcb/core/job/netlistoutputjob.h>
#include <librepcb/core/job/pickplaceoutputjob.h>
#include <librepcb/core/job/projectjsonoutputjob.h>
#include <librepcb/core/project/outputjobrunner.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

OutputJobsDialog::OutputJobsDialog(const WorkspaceSettings& settings,
                                   Project& project, UndoStack& undoStack,
                                   const QString& settingsPrefix,
                                   QWidget* parent) noexcept
  : QDialog(parent),
    mSettings(settings),
    mProject(project),
    mUndoStack(undoStack),
    mSettingsPrefix(settingsPrefix % "/output_jobs_dialog"),
    mJobs(mProject.getOutputJobs()),
    mUi(new Ui::OutputJobsDialog),
    mOnJobsEditedSlot(*this, &OutputJobsDialog::jobListEdited) {
  mUi->setupUi(this);
  connect(mUi->btnAdd, &QToolButton::clicked, this,
          &OutputJobsDialog::addClicked);
  connect(mUi->btnCopy, &QToolButton::clicked, this,
          &OutputJobsDialog::copyClicked);
  connect(mUi->btnUp, &QToolButton::clicked, this,
          &OutputJobsDialog::moveUpClicked);
  connect(mUi->btnDown, &QToolButton::clicked, this,
          &OutputJobsDialog::moveDownClicked);
  connect(mUi->btnRemove, &QToolButton::clicked, this,
          &OutputJobsDialog::removeClicked);
  connect(mUi->btnRemoveUnknownFiles, &QToolButton::clicked, this,
          &OutputJobsDialog::removeUnknownFiles);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &OutputJobsDialog::buttonBoxClicked);

  // Initialize message widget.
  mUi->msgAddDefaultJobs->init(
      tr("Click on the %1 button below to add output jobs. Or "
         "for a quick start, <a href=\"%2\">add a default set</a> of jobs.</p>")
          .arg("<b>⨁</b>", "init"),
      mJobs.isEmpty());
  connect(mUi->msgAddDefaultJobs, &MessageWidget::linkActivated, this,
          [this](const QString& link) {
            Q_UNUSED(link);
            auto gerber = GerberExcellonOutputJob::defaultStyle();
            auto pnp = std::make_shared<PickPlaceOutputJob>();
            auto archive = std::make_shared<ArchiveOutputJob>();
            archive->setInputJobs({
                {gerber->getUuid(), QString()},
            });
            mJobs.append(GraphicsOutputJob::schematicPdf());
            mJobs.append(GraphicsOutputJob::boardAssemblyPdf());
            mJobs.append(gerber);
            mJobs.append(pnp);
            mJobs.append(std::make_shared<BomOutputJob>());
            mJobs.append(archive);
            mJobs.append(std::make_shared<LppzOutputJob>());
            updateJobsList();
          });

  // Add keyboard shortcuts.
  EditorCommandSet& cmd = EditorCommandSet::instance();
  addAction(cmd.projectOpen.createAction(
      this, this, &OutputJobsDialog::openOutputDirectory));
  addAction(
      cmd.outputJobs.createAction(this, this, [this]() { runJob(nullptr); }));
  addAction(
      cmd.itemNew.createAction(this, this, &OutputJobsDialog::addClicked));
  mUi->lstJobs->addAction(
      cmd.remove.createAction(this, this, &OutputJobsDialog::removeClicked,
                              EditorCommand::ActionFlag::WidgetShortcut));

  // Populate jobs list.
  // Hide text in list widget since text is displayed with custom item
  // widgets, but list item texts are still set for keyboard navigation.
  mUi->lstJobs->setStyleSheet(
      "QListWidget::item{"
      "  color: transparent;"
      "  selection-color: transparent;"
      "}");
  updateJobsList();
  connect(mUi->lstJobs, &QListWidget::currentItemChanged, this,
          &OutputJobsDialog::currentItemChanged);
  connect(mUi->lstJobs, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem* item) {
            if (item) {
              if (auto widget = qobject_cast<OutputJobListWidgetItem*>(
                      mUi->lstJobs->itemWidget(item))) {
                runJob(widget->getJob(), true);
              }
            }
          });
  mUi->lstJobs->setCurrentRow(0);

  // Update list on job modifications.
  mJobs.onEdited.attach(mOnJobsEditedSlot);

  // Setup messages.
  connect(mUi->btnShowMessages, &QCheckBox::toggled, this,
          [this](bool checked) {
            mUi->txtLogMessages->setVisible(checked);
            mUi->btnShowMessages->setArrowType(checked ? Qt::DownArrow
                                                       : Qt::UpArrow);
          });
  connect(mUi->txtLogMessages, &QTextBrowser::anchorClicked, this,
          [this](const QUrl& url) {
            DesktopServices ds(mSettings, this);
            ds.openLocalPath(FilePath(url.toLocalFile()));
          });
  mUi->btnShowMessages->setChecked(false);

  // Load client settings.
  QSettings cs;
  restoreGeometry(cs.value(mSettingsPrefix % "/window_geometry").toByteArray());
}

OutputJobsDialog::~OutputJobsDialog() noexcept {
  // Save client settings.
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void OutputJobsDialog::reject() noexcept {
  if (mJobs != mProject.getOutputJobs()) {
    const int ret = QMessageBox::question(
        this, tr("Discard Changes?"),
        tr("You made changes to output jobs which will be lost when closing "
           "the dialog. Are you sure to discard them?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) {
      return;
    }
  }
  QDialog::reject();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OutputJobsDialog::addClicked() noexcept {
  auto escape = [](QString str) { return str.replace("&", "&&"); };

  auto add = [this](std::shared_ptr<OutputJob> job) {
    const int index = mUi->lstJobs->currentRow();
    mJobs.insert(index, job);
    updateJobsList();
    mUi->lstJobs->setCurrentRow(index + 1);
  };

  QMenu menu;
  menu.addSection(tr("Documentation"));
  menu.addAction(QIcon(":/img/actions/pdf.png"),
                 escape(tr("Schematic PDF/Image")),
                 [&]() { add(GraphicsOutputJob::schematicPdf()); });
  menu.addAction(QIcon(":/img/actions/pdf.png"),
                 escape(tr("Board Assembly PDF/Image")),
                 [&]() { add(GraphicsOutputJob::boardAssemblyPdf()); });
  menu.addSection(tr("Production Data"));
  menu.addAction(QIcon(":/img/actions/export_gerber.png"),
                 escape(GerberExcellonOutputJob::getTypeTrStatic()),
                 [&]() { add(GerberExcellonOutputJob::defaultStyle()); });
  menu.addAction(QIcon(":/img/actions/export_gerber.png"),
                 escape(GerberExcellonOutputJob::getTypeTrStatic() % " (" %
                        tr("Protel Style") % ")"),
                 [&]() { add(GerberExcellonOutputJob::protelStyle()); });
  menu.addAction(QIcon(":/img/actions/export_pick_place_file.png"),
                 escape(PickPlaceOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<PickPlaceOutputJob>()); });
  menu.addAction(QIcon(":/img/actions/export_pick_place_file.png"),
                 escape(GerberX3OutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<GerberX3OutputJob>()); });
  menu.addAction(QIcon(":/img/places/file.png"),
                 escape(NetlistOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<NetlistOutputJob>()); });
  menu.addAction(QIcon(":/img/actions/generate_bom.png"),
                 escape(BomOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<BomOutputJob>()); });
  menu.addAction(QIcon(":/img/actions/export_step.png"),
                 escape(Board3DOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<Board3DOutputJob>()); });
  menu.addSection(tr("Generic"));
  menu.addAction(QIcon(":/img/actions/copy.png"),
                 escape(CopyOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<CopyOutputJob>()); });
  menu.addAction(QIcon(":/img/actions/export_zip.png"),
                 escape(ArchiveOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<ArchiveOutputJob>()); });
  menu.addSection("LibrePCB");
  menu.addAction(QIcon(":/img/logo/48x48.png"),
                 escape(ProjectJsonOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<ProjectJsonOutputJob>()); });
  menu.addAction(QIcon(":/img/logo/48x48.png"),
                 escape(LppzOutputJob::getTypeTrStatic()),
                 [&]() { add(std::make_shared<LppzOutputJob>()); });
  menu.exec(QCursor::pos());
}

void OutputJobsDialog::copyClicked() noexcept {
  const int index = mUi->lstJobs->currentRow() - 1;
  if (std::shared_ptr<OutputJob> job = mJobs.value(index)) {
    std::shared_ptr<OutputJob> copy = job->cloneShared();
    copy->setUuid(Uuid::createRandom());
    const QString newName = *job->getName() % " " % tr("(copy)");
    copy->setName(ElementName(cleanElementName(newName)));
    mJobs.insert(index + 1, copy);
    updateJobsList();
    mUi->lstJobs->setCurrentRow(index + 2);
  }
}

void OutputJobsDialog::moveUpClicked() noexcept {
  const int oldIndex = mUi->lstJobs->currentRow() - 1;
  const int newIndex = oldIndex - 1;
  if ((newIndex >= 0) && (newIndex < mJobs.count())) {
    mJobs.swap(oldIndex, newIndex);
    updateJobsList();
    mUi->lstJobs->setCurrentRow(newIndex + 1);
  }
}

void OutputJobsDialog::moveDownClicked() noexcept {
  const int oldIndex = mUi->lstJobs->currentRow() - 1;
  const int newIndex = oldIndex + 1;
  if ((newIndex >= 0) && (newIndex < mJobs.count())) {
    mJobs.swap(newIndex, oldIndex);
    updateJobsList();
    mUi->lstJobs->setCurrentRow(newIndex + 1);
  }
}

void OutputJobsDialog::removeClicked() noexcept {
  const int index = mUi->lstJobs->currentRow() - 1;
  if ((index >= 0) && (index < mJobs.count())) {
    mUi->lstJobs->setCurrentRow(-1);
    const Uuid uuid = mJobs.at(index)->getUuid();
    for (auto& job : mJobs) {
      job.removeDependency(uuid);
    }
    mJobs.remove(index);
    updateJobsList();
    mUi->lstJobs->setCurrentRow(std::min(index + 1, mUi->lstJobs->count() - 1));
  }
}

void OutputJobsDialog::openOutputDirectory() noexcept {
  OutputJobRunner runner(mProject);
  QDir().mkpath(runner.getOutputDirectory().toStr());

  DesktopServices ds(mSettings, this);
  ds.openLocalPath(runner.getOutputDirectory());
}

void OutputJobsDialog::removeUnknownFiles() noexcept {
  bool logStarted = false;
  auto startLog = [this, &logStarted]() {
    if (!logStarted) {
      mUi->txtLogMessages->clear();
      mUi->btnShowMessages->setChecked(true);
      logStarted = true;
    }
  };

  try {
    OutputDirectoryWriter writer(mProject.getCurrentOutputDir());
    writer.loadIndex();  // can throw
    const QList<FilePath> files = writer.findUnknownFiles(mJobs.getUuidSet());
    if (files.isEmpty()) {
      startLog();
      writeLogLine(tr("No unknown files in output directory."));
    } else {
      const int linesLimit = std::min(int(files.count()), 15);
      const int remainingFiles =
          (files.count() <= linesLimit) ? 0 : (files.count() + 1 - linesLimit);
      QString text = tr("Are you sure to remove the following files?");
      text += "\n\n";
      for (int i = 0; i < linesLimit; ++i) {
        // Print relative to output folder instead of project, to reduce
        // wrapping lines due to small dialog window.
        text += QString(" • %1\n").arg(
            files.at(i).toRelative(mProject.getCurrentOutputDir()));
      }
      if (remainingFiles > 0) {
        text += QString(tr(" • And %1 more files!")).arg(remainingFiles);
      }
      const QMessageBox::StandardButton answer = QMessageBox::question(
          this, tr("Remove Unknown Files"), text,
          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
      if (answer == QMessageBox::Yes) {
        startLog();
        writeTitleLine(tr("Remove Unknown Files"));
        connect(&writer, &OutputDirectoryWriter::aboutToRemoveFile,
                [this](const FilePath& fp) {
                  writeLogLine(fp.toRelative(mProject.getPath()));
                });
        writer.removeUnknownFiles(files);  // can throw
        writer.storeIndex();  // can throw
        writeSuccessLine();
      }
    }
  } catch (const Exception& e) {
    startLog();
    writeErrorLine(e.getMsg());
  }
}

void OutputJobsDialog::runJob(std::shared_ptr<OutputJob> job,
                              bool open) noexcept {
  QVector<std::shared_ptr<OutputJob>> jobs;
  if (job) {
    // Auto-add valid dependencies of archive job.
    for (int i = 0; (i < mJobs.count()) && (mJobs[i] != job); ++i) {
      if (job->getDependencies().contains(mJobs[i]->getUuid())) {
        jobs.append(mJobs[i]);
      }
    }
    jobs.append(job);
  } else {
    jobs = mJobs.values();
  }
  QMap<std::shared_ptr<const OutputJob>, QPointer<OutputJobListWidgetItem>>
      widgets;
  for (int i = 0; i < mUi->lstJobs->count(); ++i) {
    if (QListWidgetItem* item = mUi->lstJobs->item(i)) {
      if (auto widget = qobject_cast<OutputJobListWidgetItem*>(
              mUi->lstJobs->itemWidget(item))) {
        widget->setStatusColor(Qt::transparent);
        widgets.insert(widget->getJob(), widget);
      }
    }
  }

  const bool messagesWereHidden = !mUi->btnShowMessages->isChecked();
  mUi->txtLogMessages->clear();
  mUi->btnShowMessages->setChecked(true);
  setEnabled(false);

  QPointer<OutputJobListWidgetItem> currentWidget;
  auto setCurrentStatus = [&](const QColor& color) {
    if (currentWidget) {
      currentWidget->setStatusColor(color);
    }
  };

  try {
    bool warnings = false;
    OutputJobRunner runner(mProject);
    connect(&runner, &OutputJobRunner::jobStarted, this,
            [&](std::shared_ptr<const OutputJob> j) {
              currentWidget = widgets.value(j);
              setCurrentStatus(QColor(0, 255, 0));  // green
              writeTitleLine(*j->getName());
            });
    connect(&runner, &OutputJobRunner::warning, this, [&](const QString& msg) {
      writeWarningLine(msg);
      setCurrentStatus(QColor(255, 165, 0));  // orange
      warnings = true;
    });
    connect(&runner, &OutputJobRunner::aboutToWriteFile, this,
            [&](const FilePath& fp) { writeOutputFileLine(fp); });
    connect(&runner, &OutputJobRunner::aboutToRemoveFile, this,
            [&](const FilePath& fp) {
              writeStrikeThroughLine(fp.toRelative(mProject.getPath()));
            });
    runner.run(jobs);  // can throw
    currentWidget = nullptr;
    const QList<FilePath> unknownFiles =
        runner.findUnknownFiles(mJobs.getUuidSet());  // can throw
    if (!unknownFiles.isEmpty()) {
      writeLogLine(QString("<span "
                           "style=\"text-decoration:underline;font-weight:bold;"
                           "color:DarkRed;\">%1:</span>")
                       .arg(tr("Unknown files in output folder")));
      foreach (const FilePath& fp, Toolbox::sorted(unknownFiles)) {
        writeUnknownFileLine(fp);
      }
    }
    if (warnings) {
      writeWarningLine(tr("Finished with warnings!"));
    } else {
      writeSuccessLine();
    }
    if (open) {
      // Find common base path if multiple files were generated.
      FilePath commonOutPath;
      for (auto it = runner.getWrittenFiles().begin();
           it != runner.getWrittenFiles().end(); ++it) {
        if ((!job) || (!job->getDependencies().contains(it.key()))) {
          if (!commonOutPath.isValid()) {
            commonOutPath = it.value();
          } else if (!it.value().toStr().startsWith(commonOutPath.toStr())) {
            commonOutPath = it.value().getParentDir();
          }
        }
      }
      if (commonOutPath.isValid() && (!commonOutPath.isRoot())) {
        DesktopServices ds(mSettings, this);
        ds.openLocalPath(commonOutPath);
        if (messagesWereHidden) {
          mUi->btnShowMessages->setChecked(false);
        }
      }
    }
  } catch (const Exception& e) {
    writeErrorLine(e.getMsg());
    setCurrentStatus(Qt::red);
  }

  setEnabled(true);
}

void OutputJobsDialog::currentItemChanged(QListWidgetItem* current,
                                          QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
  if (!current) {
    return;
  }

  if (OutputJobListWidgetItem* widget = qobject_cast<OutputJobListWidgetItem*>(
          mUi->lstJobs->itemWidget(current))) {
    std::shared_ptr<OutputJob> job = widget->getJob();
    if (job) {
      if (auto j = std::dynamic_pointer_cast<GraphicsOutputJob>(job)) {
        mUi->scrollArea->setWidget(new GraphicsOutputJobWidget(
            mProject, j, mSettings.defaultLengthUnit.get(), mSettingsPrefix));
      } else if (auto j =
                     std::dynamic_pointer_cast<GerberExcellonOutputJob>(job)) {
        auto widget = new GerberExcellonOutputJobWidget(mProject, j);
        connect(widget, &GerberExcellonOutputJobWidget::openUrlRequested, this,
                [this](const QUrl& url) {
                  DesktopServices ds(mSettings, this);
                  ds.openWebUrl(url);
                });
        connect(widget, &GerberExcellonOutputJobWidget::orderPcbDialogTriggered,
                this, &OutputJobsDialog::orderPcbDialogTriggered);
        mUi->scrollArea->setWidget(widget);
      } else if (auto j = std::dynamic_pointer_cast<PickPlaceOutputJob>(job)) {
        mUi->scrollArea->setWidget(new PickPlaceOutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<GerberX3OutputJob>(job)) {
        mUi->scrollArea->setWidget(new GerberX3OutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<NetlistOutputJob>(job)) {
        mUi->scrollArea->setWidget(new NetlistOutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<BomOutputJob>(job)) {
        mUi->scrollArea->setWidget(new BomOutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<Board3DOutputJob>(job)) {
        mUi->scrollArea->setWidget(new Board3DOutputJobWidget(mProject, j));
      } else if (auto j =
                     std::dynamic_pointer_cast<ProjectJsonOutputJob>(job)) {
        mUi->scrollArea->setWidget(new ProjectJsonOutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<LppzOutputJob>(job)) {
        mUi->scrollArea->setWidget(new LppzOutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<CopyOutputJob>(job)) {
        mUi->scrollArea->setWidget(new CopyOutputJobWidget(mProject, j));
      } else if (auto j = std::dynamic_pointer_cast<ArchiveOutputJob>(job)) {
        mUi->scrollArea->setWidget(
            new ArchiveOutputJobWidget(mProject, mJobs, j));
      } else {
        QLabel* widget =
            new QLabel(tr("Unknown job type. You may need a more recent "
                          "LibrePCB version to modify this job."));
        widget->setWordWrap(true);
        widget->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        QFont font = widget->font();
        font.setPointSize(12);
        widget->setFont(font);
        mUi->scrollArea->setWidget(widget);
      }
    } else {
      mUi->scrollArea->setWidget(new OutputJobHomeWidget(mSettings, mProject));
    }
    mUi->btnCopy->setEnabled(static_cast<bool>(job));
    mUi->btnUp->setEnabled(static_cast<bool>(job));
    mUi->btnDown->setEnabled(static_cast<bool>(job));
    mUi->btnRemove->setEnabled(static_cast<bool>(job));
  }
}

void OutputJobsDialog::buttonBoxClicked(QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::AcceptRole:
      if (applyChanges()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

bool OutputJobsDialog::applyChanges() noexcept {
  try {
    std::unique_ptr<CmdProjectEdit> cmd(new CmdProjectEdit(mProject));
    cmd->setOutputJobs(mJobs);
    mUndoStack.execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

void OutputJobsDialog::updateJobsList() noexcept {
  const int rowCount = mJobs.count() + 1;

  // Remove obsolete list items.
  while (mUi->lstJobs->count() > rowCount) {
    QListWidgetItem* item = mUi->lstJobs->item(mUi->lstJobs->count() - 1);
    Q_ASSERT(item);
    delete mUi->lstJobs->itemWidget(item);
    delete item;
  }

  // Add remaining list items.
  while (mUi->lstJobs->count() < rowCount) {
    mUi->lstJobs->addItem(new QListWidgetItem());
  }

  // Update list items.
  for (int i = 0; i < rowCount; ++i) {
    std::shared_ptr<OutputJob> job = mJobs.value(i - 1);
    QListWidgetItem* item = mUi->lstJobs->item(i);
    Q_ASSERT(item);

    // Update item widget.
    mUi->lstJobs->removeItemWidget(item);
    OutputJobListWidgetItem* widget = new OutputJobListWidgetItem(job, this);
    connect(widget, &OutputJobListWidgetItem::openDirectoryTriggered, this,
            &OutputJobsDialog::openOutputDirectory);
    connect(widget, &OutputJobListWidgetItem::runTriggered, this,
            [this](std::shared_ptr<OutputJob> job) { runJob(job); });
    item->setSizeHint(widget->sizeHint());
    mUi->lstJobs->setItemWidget(item, widget);

    // Set item text to make searching by keyboard working. However, the text
    // would mess up the look, thus it is made hidden with a stylesheet set
    // in the constructor (see above).
    item->setText(widget->getTitle());
  }
}

void OutputJobsDialog::jobListEdited(
    const OutputJobList& list, int index,
    const std::shared_ptr<const OutputJob>& obj,
    OutputJobList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(obj);
  switch (event) {
    case OutputJobList::Event::ElementAdded:
    case OutputJobList::Event::ElementRemoved:
      mUi->msgAddDefaultJobs->setActive(list.isEmpty());
      break;
    case OutputJobList::Event::ElementEdited: {
      if (auto item = mUi->lstJobs->item(index + 1)) {
        if (auto widget = qobject_cast<OutputJobListWidgetItem*>(
                mUi->lstJobs->itemWidget(item))) {
          widget->updateJobInfo();
        }
      }
    }
    default:
      break;
  }
}

void OutputJobsDialog::writeTitleLine(const QString& msg) noexcept {
  writeLogLine(QString("<span style=\"text-decoration:underline;\">%1:</span>")
                   .arg(msg));
}

void OutputJobsDialog::writeOutputFileLine(const FilePath& fp) noexcept {
  writeLogLine(QString("<a style=\"text-decoration:none;\" href=\"%1\">%2</a>")
                   .arg(fp.toQUrl().toString(QUrl::PrettyDecoded))
                   .arg(fp.toRelative(mProject.getPath())));
}

void OutputJobsDialog::writeUnknownFileLine(const FilePath& fp) noexcept {
  writeLogLine(
      QString(
          "<a style=\"text-decoration:none;color:DarkRed;\" href=\"%1\">%2</a>")
          .arg(fp.toQUrl().toString(QUrl::PrettyDecoded))
          .arg(fp.toRelative(mProject.getPath())));
}

void OutputJobsDialog::writeStrikeThroughLine(const QString& msg) noexcept {
  writeLogLine(QString("<s>%1</s>").arg(msg));
}

void OutputJobsDialog::writeWarningLine(const QString& msg) noexcept {
  writeLogLine(
      QString("<span style=\"color:orange;font-weight:bold;\">%1</span>")
          .arg(msg));
}

void OutputJobsDialog::writeErrorLine(const QString& msg) noexcept {
  writeLogLine(QString("<span style=\"color:red;font-weight:bold;\">%1</span>")
                   .arg(tr("ERROR") % ": " % msg));
}

void OutputJobsDialog::writeSuccessLine() noexcept {
  writeLogLine(
      QString("<span style=\"color:green;font-weight:bold;\">%1</span>")
          .arg(tr("SUCCESS!")));
}

void OutputJobsDialog::writeLogLine(const QString& line) noexcept {
  mUi->txtLogMessages->append(line);
  mUi->txtLogMessages->setMaximumHeight(
      mUi->txtLogMessages->document()->size().height());
  mUi->txtLogMessages->verticalScrollBar()->setValue(
      mUi->txtLogMessages->verticalScrollBar()->maximum());
  qApp->processEvents();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
