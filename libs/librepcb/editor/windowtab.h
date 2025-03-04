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

#ifndef LIBREPCB_EDITOR_WINDOWTAB_H
#define LIBREPCB_EDITOR_WINDOWTAB_H

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
                     int objIndex, QObject* parent = nullptr) noexcept;
  virtual ~WindowTab() noexcept;

  // General Methods
  virtual ui::TabData getBaseUiData() const noexcept = 0;
  std::shared_ptr<ProjectEditor> getProject() noexcept { return mProject; }
  int getObjIndex() const noexcept { return mObjIndex; }
  virtual void activate() noexcept {}
  virtual void deactivate() noexcept {}
  virtual bool actionTriggered(ui::ActionId id) noexcept {
    Q_UNUSED(id);
    return false;
  }
  virtual slint::Image renderScene(float width, float height) noexcept {
    Q_UNUSED(width);
    Q_UNUSED(height);
    return slint::Image();
  }
  virtual bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept {
    Q_UNUSED(pos);
    Q_UNUSED(globalPos);
    Q_UNUSED(e);
    return false;
  }
  virtual bool processSceneScrolled(
      float x, float y, slint::private_api::PointerScrollEvent e) noexcept {
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(e);
    return false;
  }
  virtual void zoomFit(float width, float height) noexcept {
    Q_UNUSED(width);
    Q_UNUSED(height);
  }
  virtual void zoomIn(float width, float height) noexcept {
    Q_UNUSED(width);
    Q_UNUSED(height);
  }
  virtual void zoomOut(float width, float height) noexcept {
    Q_UNUSED(width);
    Q_UNUSED(height);
  }

  // Operator Overloadings
  WindowTab& operator=(const WindowTab& rhs) = delete;

signals:
  void cursorCoordinatesChanged(const Point& pos, const LengthUnit& unit);
  void statusBarMessageChanged(const QString& message, int timeoutMs);
  void requestClose();
  void baseUiDataChanged();
  void tabUiDataChanged();

protected:
  GuiApplication& mApp;
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
