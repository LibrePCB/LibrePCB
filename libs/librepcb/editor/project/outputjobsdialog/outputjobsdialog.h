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

#ifndef LIBREPCB_EDITOR_OUTPUTJOBSDIALOG_H
#define LIBREPCB_EDITOR_OUTPUTJOBSDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/job/outputjob.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;
class WorkspaceSettings;

namespace editor {

class UndoStack;

namespace Ui {
class OutputJobsDialog;
}

/*******************************************************************************
 *  Class OutputJobsDialog
 ******************************************************************************/

/**
 * @brief The OutputJobsDialog class
 */
class OutputJobsDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  OutputJobsDialog() = delete;
  OutputJobsDialog(const OutputJobsDialog& other) = delete;
  explicit OutputJobsDialog(const WorkspaceSettings& settings, Project& project,
                            UndoStack& undoStack,
                            QWidget* parent = nullptr) noexcept;
  ~OutputJobsDialog() noexcept;

  // General Methods
  void preselectJobByType(const QString& typeName) noexcept;
  virtual void reject() noexcept override;

  // Operator Overloads
  OutputJobsDialog& operator=(const OutputJobsDialog& rhs) = delete;

signals:
  void orderPcbDialogTriggered();

private:  // Methods
  void addClicked() noexcept;
  void copyClicked() noexcept;
  void moveUpClicked() noexcept;
  void moveDownClicked() noexcept;
  void removeClicked() noexcept;
  void openOutputDirectory() noexcept;
  void removeUnknownFiles() noexcept;
  void runJob(std::shared_ptr<OutputJob> job, bool open = false) noexcept;
  void currentItemChanged(QListWidgetItem* current,
                          QListWidgetItem* previous) noexcept;
  void buttonBoxClicked(QAbstractButton* button) noexcept;
  bool applyChanges() noexcept;
  void updateJobsList() noexcept;
  void jobListEdited(const OutputJobList& list, int index,
                     const std::shared_ptr<const OutputJob>& obj,
                     OutputJobList::Event event) noexcept;
  void writeTitleLine(const QString& msg) noexcept;
  void writeOutputFileLine(const FilePath& fp) noexcept;
  void writeUnknownFileLine(const FilePath& fp) noexcept;
  void writeStrikeThroughLine(const QString& msg) noexcept;
  void writeWarningLine(const QString& msg) noexcept;
  void writeErrorLine(const QString& msg) noexcept;
  void writeSuccessLine() noexcept;
  void writeLogLine(const QString& line) noexcept;

private:  // Data
  const WorkspaceSettings& mSettings;
  Project& mProject;
  UndoStack& mUndoStack;
  OutputJobList mJobs;
  QScopedPointer<Ui::OutputJobsDialog> mUi;

  // Slots
  OutputJobList::OnEditedSlot mOnJobsEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
