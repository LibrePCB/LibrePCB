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

#ifndef LIBREPCB_EDITOR_PROJECTLIBRARYTAB_H
#define LIBREPCB_EDITOR_PROJECTLIBRARYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class Project;

namespace editor {

class GuiApplication;
class ProjectEditor;
class ProjectLibraryModel;

/*******************************************************************************
 *  Class ProjectLibraryTab
 ******************************************************************************/

/**
 * @brief The ProjectLibraryTab class
 */
class ProjectLibraryTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<ProjectLibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  ProjectLibraryTab() = delete;
  ProjectLibraryTab(const ProjectLibraryTab& other) = delete;
  explicit ProjectLibraryTab(GuiApplication& app, ProjectEditor& editor,
                             QObject* parent = nullptr) noexcept;
  ~ProjectLibraryTab() noexcept override;

  // General Methods
  int getProjectIndex() const noexcept;
  int getProjectObjectIndex() const noexcept { return 0; }
  ui::TabData getUiData() const noexcept override;
  ui::ProjectLibraryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::ProjectLibraryTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  ProjectLibraryTab& operator=(const ProjectLibraryTab& rhs) = delete;

private:
  void openTriggered(ui::LibraryTreeViewItemType type, const FilePath& fp);

private:
  ProjectEditor& mProjectEditor;
  Project& mProject;
  std::shared_ptr<ProjectLibraryModel> mModel;
  bool mAllChecked;
  float mViewportY;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
