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

#ifndef LIBREPCB_EDITOR_OUTPUTJOBLISTWIDGETITEM_H
#define LIBREPCB_EDITOR_OUTPUTJOBLISTWIDGETITEM_H

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

class OutputJob;
class Project;

namespace editor {

namespace Ui {
class OutputJobListWidgetItem;
}

/*******************************************************************************
 *  Class OutputJobListWidgetItem
 ******************************************************************************/

/**
 * @brief The OutputJobListWidgetItem class
 */
class OutputJobListWidgetItem final : public QFrame {
  Q_OBJECT

public:
  // Constructors / Destructor
  OutputJobListWidgetItem() = delete;
  OutputJobListWidgetItem(const OutputJobListWidgetItem& other) = delete;
  explicit OutputJobListWidgetItem(std::shared_ptr<OutputJob> job,
                                   QWidget* parent = nullptr) noexcept;
  ~OutputJobListWidgetItem() noexcept;

  // Getters
  QString getTitle() const noexcept;
  std::shared_ptr<OutputJob> getJob() noexcept { return mJob; }

  // General Methods
  void setSelected(bool selected) noexcept;
  void updateJobInfo() noexcept;
  void setStatusColor(const QColor& color) noexcept;

  // Operator Overloads
  OutputJobListWidgetItem& operator=(const OutputJobListWidgetItem& rhs) =
      delete;

signals:
  void openDirectoryTriggered();
  void runTriggered(std::shared_ptr<OutputJob> job);

private:  // Data
  std::shared_ptr<OutputJob> mJob;
  QScopedPointer<Ui::OutputJobListWidgetItem> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
