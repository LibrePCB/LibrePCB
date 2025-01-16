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

#ifndef LIBREPCB_EDITOR_INTERACTIVEHTMLBOMOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_INTERACTIVEHTMLBOMOUTPUTJOBWIDGET_H

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

class InteractiveHtmlBomOutputJob;
class Project;

namespace editor {

namespace Ui {
class InteractiveHtmlBomOutputJobWidget;
}

/*******************************************************************************
 *  Class InteractiveHtmlBomOutputJobWidget
 ******************************************************************************/

/**
 * @brief The InteractiveHtmlBomOutputJobWidget class
 */
class InteractiveHtmlBomOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  InteractiveHtmlBomOutputJobWidget() = delete;
  InteractiveHtmlBomOutputJobWidget(
      const InteractiveHtmlBomOutputJobWidget& other) = delete;
  explicit InteractiveHtmlBomOutputJobWidget(
      Project& project, std::shared_ptr<InteractiveHtmlBomOutputJob> job,
      QWidget* parent = nullptr) noexcept;
  ~InteractiveHtmlBomOutputJobWidget() noexcept;

  // Operator Overloads
  InteractiveHtmlBomOutputJobWidget& operator=(
      const InteractiveHtmlBomOutputJobWidget& rhs) = delete;

private:  // Methods
  void applyBoards(bool checked = true) noexcept;
  void applyVariants(bool checked = true) noexcept;

private:  // Data
  Project& mProject;
  std::shared_ptr<InteractiveHtmlBomOutputJob> mJob;
  QScopedPointer<Ui::InteractiveHtmlBomOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
