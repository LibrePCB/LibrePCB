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

#ifndef LIBREPCB_WSI_BASE_H
#define LIBREPCB_WSI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

class WorkspaceSettings;

/*****************************************************************************************
 *  Class WSI_Base
 ****************************************************************************************/

/**
 * @brief The WSI_Base class is the base class of all workspace settings items
 *
 * Every workspace setting is represented by a seperate object. All of these objects have
 * this class as base class. The name of all Workspace Settings Items begin with the
 * prefix "WSI_" to easily recognize them.
 *
 * @todo Use XML instead of INI files to save the workspace settings.
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_Base : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_Base(WorkspaceSettings& settings);
        virtual ~WSI_Base();

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
        WSI_Base();
        WSI_Base(const WSI_Base& other);
        WSI_Base& operator=(const WSI_Base& rhs);
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_BASE_H
