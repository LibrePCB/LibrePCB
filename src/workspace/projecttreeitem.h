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

        // Constructors / Destructor
        ProjectTreeItem(ProjectTreeItem* parent, const QDir& dir);
        ~ProjectTreeItem();

        // Getters
        const QDir& getDir()                    const {return mDir;}
        unsigned int getDepth()                 const {return mDepth;}
        int getColumnCount()                    const {return 1;}
        ProjectTreeItem* getParent()            const {return mParent;}
        ProjectTreeItem* getChild(int index)    const {return mChilds.value(index);}
        int getChildCount()                     const {return mChilds.count();}
        int getChildNumber()                    const;
        QString getName();
        bool isProject()                        const {return !mProjectFilename.isEmpty();}
        const QString& getProjectFilename()     const {return mProjectFilename;}

    private:

        // make some methods inaccessible...
        ProjectTreeItem();
        ProjectTreeItem(const ProjectTreeItem& other);
        ProjectTreeItem& operator=(const ProjectTreeItem& rhs);

        QDir mDir;
        ProjectTreeItem* mParent;
        unsigned int mDepth; ///< this is to avoid endless recursion in the parent-child relationship
        QList<ProjectTreeItem*> mChilds;
        QString mProjectFilename;
};

#endif // PROJECTTREEITEM_H
