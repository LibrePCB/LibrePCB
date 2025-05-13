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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WindowTab::WindowTab(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent), onUiDataChanged(*this), mApp(app) {
}

WindowTab::~WindowTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowTab::setUiData(const ui::TabData& data) noexcept {
  Q_UNUSED(data);
}

void WindowTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Close: {
      emit closeRequested();
      break;
    }

    default: {
      qWarning() << "Unhandled tab action:" << static_cast<int>(a);
      break;
    }
  }
}

slint::Image WindowTab::renderScene(float width, float height,
                                    int scene) noexcept {
  Q_UNUSED(width);
  Q_UNUSED(height);
  Q_UNUSED(scene);
  return slint::Image();
}

bool WindowTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  Q_UNUSED(pos);
  Q_UNUSED(e);
  return false;
}

bool WindowTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  Q_UNUSED(pos);
  Q_UNUSED(e);
  return false;
}

bool WindowTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  Q_UNUSED(e);
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
