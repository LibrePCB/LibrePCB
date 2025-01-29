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

#ifndef LIBREPCB_WINDOWSECTION_H
#define LIBREPCB_WINDOWSECTION_H

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
class WindowTab;
class WindowTabsModel;

/*******************************************************************************
 *  Class WindowSection
 ******************************************************************************/

/**
 * @brief The WindowSection class
 */
class WindowSection : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  WindowSection() = delete;
  WindowSection(const WindowSection& other) = delete;
  explicit WindowSection(GuiApplication& app,
                         QObject* parent = nullptr) noexcept;
  virtual ~WindowSection() noexcept;

  // General Methods
  const ui::WindowSectionData& getUiData() const noexcept { return mUiData; }
  std::size_t getTabCount() const noexcept;
  std::shared_ptr<WindowTab> getTab(int index) noexcept;
  std::shared_ptr<WindowTab> getCurrentTab() noexcept;
  std::shared_ptr<ProjectEditor> getCurrentProject() noexcept;
  void addTab(std::shared_ptr<WindowTab> tab) noexcept;
  void closeTab(int index) noexcept;
  void setCurrentTab(int index) noexcept;
  bool actionTriggered(ui::ActionId id) noexcept;
  slint::Image renderScene(float width, float height) noexcept;
  bool processScenePointerEvent(const QPointF& pos, const QPointF& globalPos,
                                slint::private_api::PointerEvent e) noexcept;
  bool processSceneScrolled(float x, float y,
                            slint::private_api::PointerScrollEvent e) noexcept;
  void zoomFit(float width, float height) noexcept;
  void zoomIn(float width, float height) noexcept;
  void zoomOut(float width, float height) noexcept;
  void highlight() noexcept;

  // Operator Overloadings
  WindowSection& operator=(const WindowSection& rhs) = delete;

signals:
  void uiDataChanged();
  void currentProjectChanged(std::shared_ptr<ProjectEditor> prj);
  void cursorCoordinatesChanged(const Point& pos, const LengthUnit& unit);

private:
  void requestRepaint() noexcept;

private:
  std::shared_ptr<WindowTabsModel> mTabsModel;
  ui::WindowSectionData mUiData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
