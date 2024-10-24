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
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;

namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;
class OpenGlSceneBuilder;
class OpenGlView;

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
  struct Tab {
    std::shared_ptr<ProjectEditor> project;
    ui::TabType type;
    int objIndex = -1;
    QPointF offset;
    qreal scale = 1;
    QRectF sceneRect;

    qreal projectionFov = 15;
    QPointF projectionCenter;
    QMatrix4x4 transform;
  };
  struct Section {
    QList<Tab> tabs;
    std::shared_ptr<GraphicsScene> scene;
    std::shared_ptr<OpenGlView> openGlView;
    std::shared_ptr<OpenGlSceneBuilder> openGlSceneBuilder;
    bool panning = false;
    QPointF startScenePos;

    QPointF mousePressPosition;
    QMatrix4x4 mousePressTransform;
    QPointF mousePressCenter;
    QSet<slint::private_api::PointerEventButton> buttons;
  };

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
  void board3dItemClicked(int section, int tab) noexcept;
  void addTab(ui::TabType type, const QString& title, int objIndex) noexcept;
  void tabClicked(int section, int tab) noexcept;
  void tabCloseClicked(int section, int tab) noexcept;
  slint::Image renderScene(int section, int tab, float width, float height,
                           int frame) noexcept;
  slint::private_api::EventResult onScenePointerEvent(
      int section, int tab, float x, float y,
      slint::private_api::PointerEvent e) noexcept;
  slint::private_api::EventResult onSceneScrolled(
      int section, int tab, float x, float y,
      slint::private_api::PointerScrollEvent e) noexcept;

  GuiApplication& mApp;
  slint::ComponentHandle<ui::AppWindow> mWindow;
  const ui::Globals& mGlobals;
  const int mIndex;
  std::shared_ptr<ProjectEditor> mProject;
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;
  std::unique_ptr<IF_GraphicsLayerProvider> mLayerProvider;
  QList<Section> mSections;
  std::shared_ptr<slint::VectorModel<ui::SectionData>> mSectionsData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
