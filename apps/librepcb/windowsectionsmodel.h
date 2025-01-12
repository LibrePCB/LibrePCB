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

#ifndef LIBREPCB_WINDOWSECTIONSMODEL_H
#define LIBREPCB_WINDOWSECTIONSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

class GuiApplication;
class ProjectEditor;
class WindowSection;

/*******************************************************************************
 *  Class WindowSectionsModel
 ******************************************************************************/

/**
 * @brief The WindowSectionsModel class
 */
class WindowSectionsModel : public QObject,
                            public slint::Model<ui::WindowSection> {
  Q_OBJECT

public:
  // Constructors / Destructor
  WindowSectionsModel() = delete;
  WindowSectionsModel(const WindowSectionsModel& other) = delete;
  explicit WindowSectionsModel(GuiApplication& app,
                               QObject* parent = nullptr) noexcept;
  virtual ~WindowSectionsModel() noexcept;

  // General Methods
  void openCreateLibraryTab() noexcept;
  void openSchematic(std::shared_ptr<ProjectEditor> prj, int index) noexcept;
  void openBoard(std::shared_ptr<ProjectEditor> prj, int index) noexcept;
  void openBoard3dViewer(int sectionId, int tab) noexcept;
  void splitSection(int sectionId) noexcept;
  void closeSection(int sectionId) noexcept;
  void setCurrentTab(int sectionId, int tab) noexcept;
  void closeTab(int sectionId, int tab) noexcept;
  slint::Image renderScene(int sectionId, int tab, float width, float height,
                           int frame) noexcept;
  slint::private_api::EventResult processScenePointerEvent(
      int sectionId, float x, float y, float width, float height,
      slint::private_api::PointerEvent e) noexcept;
  slint::private_api::EventResult processSceneScrolled(
      int sectionId, float x, float y, float width, float height,
      slint::private_api::PointerScrollEvent e) noexcept;
  void zoomFit(int sectionId, float width, float height) noexcept;
  void zoomIn(int sectionId, float width, float height) noexcept;
  void zoomOut(int section, float width, float height) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::WindowSection> row_data(std::size_t i) const override;

  // Operator Overloadings
  WindowSectionsModel& operator=(const WindowSectionsModel& rhs) = delete;

signals:
  void currentSectionIdChanged(int id);
  void currentProjectChanged(std::shared_ptr<ProjectEditor> prj);
  void cursorCoordinatesChanged(qreal x, qreal y);

private:
  void addTab(ui::TabType type, std::shared_ptr<ProjectEditor> prj,
              int objIndex) noexcept;
  std::shared_ptr<WindowSection> getSection(int id,
                                            int* index = nullptr) noexcept;
  void updateIndex() noexcept;

  GuiApplication& mApp;
  QList<std::shared_ptr<WindowSection>> mItems;
  QHash<int, int> mIndex;  ///< Map from section ID to #mItems index
  int mNextId;  ///< Next free section ID
  int mCurrentSectionId;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
