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

#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;

namespace editor {
namespace app {

class GuiApplication;
class ProjectEditor;
class WindowSection;
class WindowTab;

/*******************************************************************************
 *  Class WindowSectionsModel
 ******************************************************************************/

/**
 * @brief The WindowSectionsModel class
 */
class WindowSectionsModel : public QObject,
                            public slint::Model<ui::WindowSectionData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  WindowSectionsModel() = delete;
  WindowSectionsModel(const WindowSectionsModel& other) = delete;
  explicit WindowSectionsModel(GuiApplication& app, const ui::Data& uiData,
                               const QString& settingsPrefix,
                               QObject* parent = nullptr) noexcept;
  virtual ~WindowSectionsModel() noexcept;

  // General Methods
  bool actionTriggered(ui::ActionId id, int sectionIndex) noexcept;
  void openSchematic(std::shared_ptr<ProjectEditor> prj, int index) noexcept;
  void openBoard(std::shared_ptr<ProjectEditor> prj, int index) noexcept;
  void setCurrentTab(int sectionIndex, int tabIndex) noexcept;
  void closeTab(int sectionIndex, int tabIndex) noexcept;
  slint::Image renderScene(int sectionIndex, float width, float height,
                           int frame) noexcept;
  slint::private_api::EventResult processScenePointerEvent(
      int sectionIndex, const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept;
  slint::private_api::EventResult processSceneScrolled(
      int sectionIndex, float x, float y,
      slint::private_api::PointerScrollEvent e) noexcept;
  void zoomFit(int sectionIndex, float width, float height) noexcept;
  void zoomIn(int sectionIndex, float width, float height) noexcept;
  void zoomOut(int sectionIndex, float width, float height) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::WindowSectionData> row_data(std::size_t i) const override;

  // Operator Overloadings
  WindowSectionsModel& operator=(const WindowSectionsModel& rhs) = delete;

signals:
  void currentProjectChanged(std::shared_ptr<ProjectEditor> prj);
  void cursorCoordinatesChanged(const Point& pos, const LengthUnit& unit);

private:
  void splitSection(int sectionIndex) noexcept;
  void addTab(std::shared_ptr<WindowTab> tab) noexcept;
  template <typename T>
  bool switchToOpenTab(std::shared_ptr<ProjectEditor> prj,
                       int objIndex) noexcept;

  GuiApplication& mApp;
  const ui::Data& mUiData;
  const QString mSettingsPrefix;
  QList<std::shared_ptr<WindowSection>> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
