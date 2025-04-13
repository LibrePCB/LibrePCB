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

#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;
class Point;

namespace editor {

class GuiApplication;
class ProjectEditor2;

/*******************************************************************************
 *  Class WindowTab
 ******************************************************************************/

/**
 * @brief The WindowTab class
 */
class WindowTab : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<WindowTab> onUiDataChanged;

  // Constructors / Destructor
  WindowTab() = delete;
  WindowTab(const WindowTab& other) = delete;
  explicit WindowTab(GuiApplication& app, QObject* parent = nullptr) noexcept;
  virtual ~WindowTab() noexcept;

  // General Methods
  virtual ui::TabData getUiData() const noexcept = 0;
  virtual void setUiData(const ui::TabData& data) noexcept;
  virtual void activate() noexcept {}
  virtual void deactivate() noexcept {}
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
  virtual bool processSceneKeyPressed(
      const slint::private_api::KeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool processSceneKeyReleased(
      const slint::private_api::KeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }

  // Operator Overloadings
  WindowTab& operator=(const WindowTab& rhs) = delete;

signals:
  void panelPageRequested(ui::PanelPage p);
  void closeRequested();
  void cursorCoordinatesChanged(const Point& pos, const LengthUnit& unit);
  void statusBarMessageChanged(const QString& message, int timeoutMs);

protected:
  virtual void triggerAsync(ui::Action a) noexcept;

  GuiApplication& mApp;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
