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

#ifndef LIBREPCB_EDITOR_ARCHIVEOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_ARCHIVEOUTPUTJOBWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/job/outputjob.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ArchiveOutputJob;
class Project;

namespace editor {

namespace Ui {
class ArchiveOutputJobWidget;
}

/*******************************************************************************
 *  Class ArchiveOutputJobWidget
 ******************************************************************************/

/**
 * @brief The ArchiveOutputJobWidget class
 */
class ArchiveOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  ArchiveOutputJobWidget() = delete;
  ArchiveOutputJobWidget(const ArchiveOutputJobWidget& other) = delete;
  explicit ArchiveOutputJobWidget(Project& project,
                                  const OutputJobList& allJobs,
                                  std::shared_ptr<ArchiveOutputJob> job,
                                  QWidget* parent = nullptr) noexcept;
  ~ArchiveOutputJobWidget() noexcept;

  // Operator Overloads
  ArchiveOutputJobWidget& operator=(const ArchiveOutputJobWidget& rhs) = delete;

private:  // Methods
  void applyInputJobs() noexcept;

private:  // Data
  Project& mProject;
  const OutputJobList& mAllJobs;
  std::shared_ptr<ArchiveOutputJob> mJob;
  QScopedPointer<Ui::ArchiveOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
