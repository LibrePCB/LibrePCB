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

#ifndef LIBREPCB_EDITOR_OUTPUTJOBHOMEWIDGET_H
#define LIBREPCB_EDITOR_OUTPUTJOBHOMEWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;
class WorkspaceSettings;

namespace editor {

namespace Ui {
class OutputJobHomeWidget;
}

/*******************************************************************************
 *  Class OutputJobHomeWidget
 ******************************************************************************/

/**
 * @brief The OutputJobHomeWidget class
 */
class OutputJobHomeWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  OutputJobHomeWidget() = delete;
  OutputJobHomeWidget(const OutputJobHomeWidget& other) = delete;
  explicit OutputJobHomeWidget(const WorkspaceSettings& settings,
                               const Project& project,
                               QWidget* parent = nullptr) noexcept;
  ~OutputJobHomeWidget() noexcept;

  // Operator Overloads
  OutputJobHomeWidget& operator=(const OutputJobHomeWidget& rhs) = delete;

private:  // Data
  const WorkspaceSettings& mSettings;
  const Project& mProject;
  QScopedPointer<Ui::OutputJobHomeWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
