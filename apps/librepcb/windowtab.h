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

#ifndef LIBREPCB_WINDOWTAB_H
#define LIBREPCB_WINDOWTAB_H

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

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class WindowTab
 ******************************************************************************/

/**
 * @brief The WindowTab class
 */
class WindowTab : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  WindowTab() = delete;
  WindowTab(const WindowTab& other) = delete;
  explicit WindowTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                     ui::TabType type, int objIndex, const QString& title,
                     QObject* parent = nullptr) noexcept;
  virtual ~WindowTab() noexcept;

  // General Methods
  const ui::Tab& getUiData() const noexcept { return mUiData; }
  std::shared_ptr<ProjectEditor> getProject() noexcept { return mProject; }
  int getObjIndex() const noexcept { return mObjIndex; }
  virtual void activate() noexcept = 0;
  virtual void deactivate() noexcept = 0;
  virtual slint::Image renderScene(float width, float height) noexcept = 0;
  virtual bool processScenePointerEvent(
      float x, float y, float width, float height,
      slint::private_api::PointerEvent e) noexcept = 0;
  virtual bool processSceneScrolled(
      float x, float y, float width, float height,
      slint::private_api::PointerScrollEvent e) noexcept = 0;
  virtual void zoomFit(float width, float height) noexcept = 0;
  virtual void zoomIn(float width, float height) noexcept = 0;
  virtual void zoomOut(float width, float height) noexcept = 0;

  // Operator Overloadings
  WindowTab& operator=(const WindowTab& rhs) = delete;

signals:
  void cursorCoordinatesChanged(qreal x, qreal y);
  void requestRepaint();

protected:
  ui::Tab mUiData;
  std::shared_ptr<ProjectEditor> mProject;
  int mObjIndex = -1;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
