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

#ifndef LIBREPCB_EDITOR_IF_GRAPHICSVIEWEVENTHANDLER_H
#define LIBREPCB_EDITOR_IF_GRAPHICSVIEWEVENTHANDLER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

struct GraphicsSceneKeyEvent;
struct GraphicsSceneMouseEvent;

/*******************************************************************************
 *  Interface IF_GraphicsViewEventHandler
 ******************************************************************************/

/**
 * @brief The IF_GraphicsViewEventHandler class
 */
class IF_GraphicsViewEventHandler {
public:
  // Constructors / Destructor
  explicit IF_GraphicsViewEventHandler() noexcept {}
  virtual ~IF_GraphicsViewEventHandler() noexcept {}

  virtual bool graphicsSceneKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool graphicsSceneKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool graphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool graphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool graphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool graphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
  virtual bool graphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept {
    Q_UNUSED(e);
    return false;
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
