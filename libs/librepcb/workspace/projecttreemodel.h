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

#ifndef LIBREPCB_PROJECTTREEMODEL_H
#define LIBREPCB_PROJECTTREEMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace workspace {

class Workspace;

/*******************************************************************************
 *  Class ProjectTreeModel
 ******************************************************************************/

/**
 * @brief The ProjectTreeModel class
 *
 * @author ubruhin
 *
 * @date 2014-06-24
 */
class ProjectTreeModel : public QFileSystemModel {
public:
  // Constructors / Destructor
  ProjectTreeModel()                              = delete;
  ProjectTreeModel(const ProjectTreeModel& other) = delete;
  explicit ProjectTreeModel(const Workspace& workspace,
                            QObject*         parent = nullptr) noexcept;
  ~ProjectTreeModel() noexcept;

  // General methods
  QModelIndexList getPersistentIndexList() const {
    return persistentIndexList();
  }
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;

  // Operator Overloadings
  ProjectTreeModel& operator=(const ProjectTreeModel& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif  // LIBREPCB_PROJECTTREEMODEL_H
