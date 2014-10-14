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

#ifndef WORKSPACESETTINGSITEM_H
#define WORKSPACESETTINGSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class WorkspaceSettings;

/*****************************************************************************************
 *  Class WorkspaceSettingsItem
 ****************************************************************************************/

/**
 * @brief The WorkspaceSettingsItem class is the base class of all workspace settings items
 *
 * Every workspace setting is represented by a seperate object. All of these objects have
 * this class as base class. The name of all subclasses of WorkspaceSettingsItem begin
 * with the prefix "WSI_" to easily recognize them.
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WorkspaceSettingsItem : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WorkspaceSettingsItem(WorkspaceSettings& settings);
        virtual ~WorkspaceSettingsItem();

        // General Methods
        virtual void restoreDefault() = 0;
        virtual void apply() = 0;
        virtual void revert() = 0;


    protected:

        // Helper Methods for Derived Classes
        void saveValue(const QString& key, const QVariant& value);
        QVariant loadValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

        // General Attributes
        WorkspaceSettings& mSettings;

    private:

        // make some methods inaccessible...
        WorkspaceSettingsItem();
        WorkspaceSettingsItem(const WorkspaceSettingsItem& other);
        WorkspaceSettingsItem& operator=(const WorkspaceSettingsItem& rhs);
};

#endif // WORKSPACESETTINGSITEM_H
