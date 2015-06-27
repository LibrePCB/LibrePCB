/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef FAVORITEPROJECTSMODEL_H
#define FAVORITEPROJECTSMODEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

/*****************************************************************************************
 *  Class FavoriteProjectsModel
 ****************************************************************************************/

/**
 * @brief The FavoriteProjectsModel class
 */
class FavoriteProjectsModel : public QAbstractListModel
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        FavoriteProjectsModel(const Workspace& workspace);
        ~FavoriteProjectsModel();

        // General Methods
        bool isFavoriteProject(const FilePath& filepath) const;
        void addFavoriteProject(const FilePath& filepath);
        void removeFavoriteProject(const FilePath& filepath);

    private:

        // make some methods inaccessible...
        FavoriteProjectsModel(const FavoriteProjectsModel& other);
        FavoriteProjectsModel& operator=(const FavoriteProjectsModel& rhs);

        // General Methods
        void save();

        // Inherited Methods
        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

        // Attributes
        const Workspace& mWorkspace;
        QList<FilePath> mFavoriteProjects;
};

#endif // FAVORITEPROJECTSMODEL_H
