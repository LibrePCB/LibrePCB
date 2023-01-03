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

#ifndef LIBREPCB_EDITOR_BOARDSIDESELECTORWIDGET_H
#define LIBREPCB_EDITOR_BOARDSIDESELECTORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/pkg/footprintpad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class BoardSideSelectorWidget
 ******************************************************************************/

/**
 * @brief The BoardSideSelectorWidget class
 */
class BoardSideSelectorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BoardSideSelectorWidget(QWidget* parent = nullptr) noexcept;
  BoardSideSelectorWidget(const BoardSideSelectorWidget& other) = delete;
  ~BoardSideSelectorWidget() noexcept;

  // Getters
  FootprintPad::ComponentSide getCurrentBoardSide() const noexcept;

  // Setters
  void setCurrentBoardSide(FootprintPad::ComponentSide side) noexcept;
  void setBoardSideTop() noexcept {
    setCurrentBoardSide(FootprintPad::ComponentSide::Top);
  }
  void setBoardSideBottom() noexcept {
    setCurrentBoardSide(FootprintPad::ComponentSide::Bottom);
  }

  // Operator Overloadings
  BoardSideSelectorWidget& operator=(const BoardSideSelectorWidget& rhs) =
      delete;

signals:
  void currentBoardSideChanged(FootprintPad::ComponentSide side);

private:  // Methods
  void btnTopToggled(bool checked) noexcept;
  void btnBottomToggled(bool checked) noexcept;

private:  // Data
  QToolButton* mBtnTop;
  QToolButton* mBtnBottom;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
