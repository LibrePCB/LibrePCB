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

#ifndef LIBREPCB_MAINWINDOW_H
#define LIBREPCB_MAINWINDOW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class MainWindow
 ******************************************************************************/

/**
 * @brief The MainWindow class
 */
class MainWindow : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  MainWindow() = delete;
  MainWindow(const MainWindow& other) = delete;
  explicit MainWindow(GuiApplication& app,
                      slint::ComponentHandle<ui::AppWindow> win, int index,
                      QObject* parent = nullptr) noexcept;
  virtual ~MainWindow() noexcept;

  // Operator Overloadings
  MainWindow& operator=(const MainWindow& rhs) = delete;

private:
  void projectItemDoubleClicked(const slint::SharedString& path) noexcept;
  void schematicItemClicked(int index) noexcept;
  void boardItemClicked(int index) noexcept;
  void tabClicked(int group, int index) noexcept;
  slint::Image renderScene(int section, float width, float height,
                           int frame) noexcept;
  slint::private_api::EventResult onScnePointerEvent(
      int section, float x1, float y1, float x0, float y0,
      slint::private_api::PointerEvent e) noexcept;
  slint::private_api::EventResult onSceneScrolled(
      int section, float x, float y,
      slint::private_api::PointerScrollEvent e) noexcept;

  GuiApplication& mApp;
  slint::ComponentHandle<ui::AppWindow> mWindow;
  const ui::Globals& mGlobals;
  const int mIndex;
  std::shared_ptr<ProjectEditor> mProject;
  std::unique_ptr<IF_GraphicsLayerProvider> mLayerProvider;
  QVector<std::shared_ptr<slint::VectorModel<ui::Tab>>> mTabs;  ///< count=2
  QVector<std::shared_ptr<GraphicsScene>> mScenes;  ///< count=2
  QVector<QTransform> mOldTransforms;
  QVector<QTransform> mTransforms;
  QVector<bool> mMoving;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
