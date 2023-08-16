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

#ifndef LIBREPCB_EDITOR_BOARD3DOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_BOARD3DOUTPUTJOBWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board3DOutputJob;
class Project;

namespace editor {

namespace Ui {
class Board3DOutputJobWidget;
}

/*******************************************************************************
 *  Class Board3DOutputJobWidget
 ******************************************************************************/

/**
 * @brief The Board3DOutputJobWidget class
 */
class Board3DOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  Board3DOutputJobWidget() = delete;
  Board3DOutputJobWidget(const Board3DOutputJobWidget& other) = delete;
  explicit Board3DOutputJobWidget(Project& project,
                                  std::shared_ptr<Board3DOutputJob> job,
                                  QWidget* parent = nullptr) noexcept;
  ~Board3DOutputJobWidget() noexcept;

  // Operator Overloads
  Board3DOutputJobWidget& operator=(const Board3DOutputJobWidget& rhs) = delete;

private:  // Methods
  void applyBoards(bool checked = true) noexcept;
  void applyVariants(bool checked = true) noexcept;

private:  // Data
  Project& mProject;
  std::shared_ptr<Board3DOutputJob> mJob;
  QScopedPointer<Ui::Board3DOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
