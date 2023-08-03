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

#ifndef LIBREPCB_EDITOR_LPPZOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_LPPZOUTPUTJOBWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/job/outputjob.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LppzOutputJob;
class Project;

namespace editor {

namespace Ui {
class LppzOutputJobWidget;
}

/*******************************************************************************
 *  Class LppzOutputJobWidget
 ******************************************************************************/

/**
 * @brief The LppzOutputJobWidget class
 */
class LppzOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  LppzOutputJobWidget() = delete;
  LppzOutputJobWidget(const LppzOutputJobWidget& other) = delete;
  explicit LppzOutputJobWidget(Project& project,
                               std::shared_ptr<LppzOutputJob> job,
                               QWidget* parent = nullptr) noexcept;
  ~LppzOutputJobWidget() noexcept;

  // Operator Overloads
  LppzOutputJobWidget& operator=(const LppzOutputJobWidget& rhs) = delete;

private:  // Data
  Project& mProject;
  std::shared_ptr<LppzOutputJob> mJob;
  QScopedPointer<Ui::LppzOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
