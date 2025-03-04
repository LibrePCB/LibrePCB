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

#ifndef LIBREPCB_EDITOR_PROJECTSMODEL_H
#define LIBREPCB_EDITOR_PROJECTSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {
namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class ProjectsModel
 ******************************************************************************/

/**
 * @brief The ProjectsModel class
 */
class ProjectsModel : public QObject, public slint::Model<ui::ProjectData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectsModel() = delete;
  ProjectsModel(const ProjectsModel& other) = delete;
  explicit ProjectsModel(GuiApplication& app,
                         QObject* parent = nullptr) noexcept;
  virtual ~ProjectsModel() noexcept;

  // General Methods
  int getIndexOf(std::shared_ptr<ProjectEditor> prj) noexcept;
  std::shared_ptr<ProjectEditor> getProject(int index) noexcept;
  std::shared_ptr<ProjectEditor> openProject(FilePath fp = FilePath()) noexcept;
  void closeProject(int index) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ProjectData> row_data(std::size_t i) const override;

  // Operator Overloadings
  ProjectsModel& operator=(const ProjectsModel& rhs) = delete;

private:
  /**
   * @brief Ask the user whether to restore a backup of a project
   *
   * @param dir   The project directory to be restored.
   *
   * @retval true   Restore backup.
   * @retval false  Do not restore backup.
   *
   * @throw Exception to abort opening the project.
   */
  static bool askForRestoringBackup(const FilePath& dir);

  GuiApplication& mApp;
  QList<std::shared_ptr<ProjectEditor>> mEditors;
  std::vector<ui::ProjectData> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
