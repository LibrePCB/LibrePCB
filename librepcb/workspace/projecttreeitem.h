/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECTTREEITEM_H
#define PROJECTTREEITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Project;

/*****************************************************************************************
 *  Class ProjectTreeItem
 ****************************************************************************************/

/**
 * @brief The ProjectTreeItem class
 *
 * @author ubruhin
 *
 * @date 2014-06-24
 */
class ProjectTreeItem
{
    public:

        // Types
        enum ItemType_t {
            File,
            Folder,
            ProjectFile,
            ProjectFolder,
        };

        // Constructors / Destructor
        ProjectTreeItem(ProjectTreeItem* parent, const FilePath& filepath);
        ~ProjectTreeItem();

        // Getters
        ItemType_t getType()                    const {return mType;}
        const FilePath& getFilePath()           const {return mFilePath;}
        unsigned int getDepth()                 const {return mDepth;}
        int getColumnCount()                    const {return 1;}
        ProjectTreeItem* getParent()            const {return mParent;}
        ProjectTreeItem* getChild(int index)    const {return mChilds.value(index);}
        int getChildCount()                     const {return mChilds.count();}
        int getChildNumber()                    const;
        QVariant data(int role) const;

    private:

        // make some methods inaccessible...
        ProjectTreeItem();
        ProjectTreeItem(const ProjectTreeItem& other);
        ProjectTreeItem& operator=(const ProjectTreeItem& rhs);

        FilePath mFilePath;
        ProjectTreeItem* mParent;
        ItemType_t mType;
        QMimeType mMimeType;
        unsigned int mDepth; ///< this is to avoid endless recursion in the parent-child relationship
        QList<ProjectTreeItem*> mChilds;
};

#endif // PROJECTTREEITEM_H
