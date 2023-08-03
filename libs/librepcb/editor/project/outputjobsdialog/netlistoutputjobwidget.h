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

#ifndef LIBREPCB_EDITOR_NETLISTOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_NETLISTOUTPUTJOBWIDGET_H

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

class NetlistOutputJob;
class Project;

namespace editor {

namespace Ui {
class NetlistOutputJobWidget;
}

/*******************************************************************************
 *  Class NetlistOutputJobWidget
 ******************************************************************************/

/**
 * @brief The NetlistOutputJobWidget class
 */
class NetlistOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  NetlistOutputJobWidget() = delete;
  NetlistOutputJobWidget(const NetlistOutputJobWidget& other) = delete;
  explicit NetlistOutputJobWidget(Project& project,
                                  std::shared_ptr<NetlistOutputJob> job,
                                  QWidget* parent = nullptr) noexcept;
  ~NetlistOutputJobWidget() noexcept;

  // Operator Overloads
  NetlistOutputJobWidget& operator=(const NetlistOutputJobWidget& rhs) = delete;

private:  // Methods
  void applyBoards(bool checked = true) noexcept;

private:  // Data
  Project& mProject;
  std::shared_ptr<NetlistOutputJob> mJob;
  QScopedPointer<Ui::NetlistOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
