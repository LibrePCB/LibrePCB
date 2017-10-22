/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "projecttreemodel.h"
#include "fileiconprovider.h"
#include "workspace.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectTreeModel::ProjectTreeModel(const Workspace& workspace, QObject* parent) noexcept :
    QFileSystemModel(parent)
{
    setIconProvider(new FileIconProvider());
    setRootPath(workspace.getProjectsPath().toStr());
}

ProjectTreeModel::~ProjectTreeModel() noexcept
{
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

QVariant ProjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role == Qt::DisplayRole) && (orientation == Qt::Horizontal) && (section == 0)){
        return QString("Workspace Projects");
    } else {
        return QFileSystemModel::headerData(section, orientation, role);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
