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

#ifndef LIBREPCB_PROJECT_SCHEMATICPAGESDOCK_H
#define LIBREPCB_PROJECT_SCHEMATICPAGESDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Project;

namespace editor {

namespace Ui {
class SchematicPagesDock;
}

/*******************************************************************************
 *  Class SchematicPagesDock
 ******************************************************************************/

/**
 * @brief The SchematicPagesDock class
 */
class SchematicPagesDock final : public QDockWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicPagesDock()                                = delete;
  SchematicPagesDock(const SchematicPagesDock& other) = delete;
  SchematicPagesDock(Project& project, QWidget* parent = nullptr);
  ~SchematicPagesDock();

  // General Methods
  void setSelectedSchematic(int index) noexcept;

  // Operator Overloadings
  SchematicPagesDock& operator=(const SchematicPagesDock& rhs) = delete;

signals:
  void selectedSchematicChanged(int index);
  void addSchematicTriggered();
  void removeSchematicTriggered(int index);

protected:
  void resizeEvent(QResizeEvent* event) noexcept override;
  bool eventFilter(QObject* obj, QEvent* event) noexcept override;

private:  // Methods
  void removeSelectedSchematic() noexcept;
  void schematicAdded(int newIndex) noexcept;
  void schematicRemoved(int oldIndex) noexcept;

private:  // Data
  Project&                               mProject;
  QScopedPointer<Ui::SchematicPagesDock> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SCHEMATICPAGESDOCK_H
