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

#ifndef LIBREPCB_EDITOR_GERBEREXCELLONOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_GERBEREXCELLONOUTPUTJOBWIDGET_H

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

class GerberExcellonOutputJob;
class Project;

namespace editor {

namespace Ui {
class GerberExcellonOutputJobWidget;
}

/*******************************************************************************
 *  Class GerberExcellonOutputJobWidget
 ******************************************************************************/

/**
 * @brief The GerberExcellonOutputJobWidget class
 */
class GerberExcellonOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  GerberExcellonOutputJobWidget() = delete;
  GerberExcellonOutputJobWidget(const GerberExcellonOutputJobWidget& other) =
      delete;
  explicit GerberExcellonOutputJobWidget(
      Project& project, std::shared_ptr<GerberExcellonOutputJob> job,
      QWidget* parent = nullptr) noexcept;
  ~GerberExcellonOutputJobWidget() noexcept;

  // Operator Overloads
  GerberExcellonOutputJobWidget& operator=(
      const GerberExcellonOutputJobWidget& rhs) = delete;

signals:
  void openUrlRequested(const QUrl& url);
  void orderPcbDialogTriggered();

private:  // Methods
  void applyBoards(bool checked = true) noexcept;

private:  // Data
  Project& mProject;
  std::shared_ptr<GerberExcellonOutputJob> mJob;
  QScopedPointer<Ui::GerberExcellonOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
