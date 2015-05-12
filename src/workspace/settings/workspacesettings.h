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

#ifndef WORKSPACESETTINGS_H
#define WORKSPACESETTINGS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/file_io/filepath.h"

// All Settings Classes
#include "items/wsi_applocale.h"
#include "items/wsi_appdefaultmeasurementunits.h"
#include "items/wsi_projectautosaveinterval.h"
#include "items/wsi_librarylocaleorder.h"
#include "items/wsi_librarynormorder.h"
#include "items/wsi_debugtools.h"
#include "items/wsi_appearance.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class WorkspaceSettingsDialog;

/*****************************************************************************************
 *  Class WorkspaceSettings
 ****************************************************************************************/

/**
 * @brief The WorkspaceSettings class manages all workspace related settings (and more)
 *
 * The ".metadata" directory in a workspace is used to store workspace related settings
 * and other workspace related stuff. This class is an interface to such workspace related
 * stuff. A WorkspaceSettings object is created in the constructor of the Workspace object.
 * As there can be only one Workspace object in an application instance, there is also
 * only one WorkspaceSettings object in an application instance. Never create more
 * WorkspaceSettings objects!
 *
 * This class also provides a graphical dialog to show and edit all these settings. For
 * this purpose, the WorkspaceSettingsDialog class is used. It can be shown by calling
 * the slot #showSettingsDialog().
 *
 * Most of the settings are stored in the file ".metadata/settings.ini" by using QSettings
 * objects. But this file can also be used without using the WorkspaceSettings class. For
 * example the position of most windows (QMainWindow/QDialog) should be stored in the
 * workspace to restore their positions after the application is closed and restarted.
 * But these values are not really settings (they are not shown in the settings dialog),
 * so it does not make sense to manage them with this class... simply use your own
 * QSettings object and pass the filepath to the "settings.ini" file an use the
 * QSettings::IniFormat parameter. To get the filepath to the "settings.ini", you can use
 * the method WorkspaceSettings#getFilepath() without any arguments.
 *
 * @author ubruhin
 * @date 2014-07-12
 *
 * @todo Maybe use XML files instead of INI to save all workspace settings...
 */
class WorkspaceSettings final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WorkspaceSettings();
        ~WorkspaceSettings();


        // Getters: General
        const FilePath& getMetadataPath() const {return mMetadataPath;}


        // Getters: Settings Items
        WSI_AppLocale* getAppLocale() const noexcept {return mAppLocale;}
        WSI_AppDefaultMeasurementUnits* getAppDefMeasUnits() const noexcept {return mAppDefMeasUnits;}
        WSI_ProjectAutosaveInterval* getProjectAutosaveInterval() const noexcept {return mProjectAutosaveInterval;}
        WSI_LibraryLocaleOrder* getLibLocaleOrder() const noexcept {return mLibraryLocaleOrder;}
        WSI_LibraryNormOrder* getLibNormOrder() const noexcept {return mLibraryNormOrder;}
        WSI_DebugTools* getDebugTools() const noexcept {return mDebugTools;}
        WSI_Appearance* getAppearance() const noexcept {return mAppearance;}


        // General Methods
        void restoreDefaults() noexcept;
        void applyAll() noexcept;
        void revertAll() noexcept;


    public slots:

        /**
         * @brief Open the workspace settings dialog
         *
         * @note The dialog is application modal, so this method is blocking while the
         * dialog is open. This method will not return before the dialog is closed.
         */
        void showSettingsDialog();


    private:

        // make some methods inaccessible...
        WorkspaceSettings(const WorkspaceSettings& other);
        WorkspaceSettings& operator=(const WorkspaceSettings& rhs);


        // General Attributes
        FilePath mMetadataPath; ///< the ".metadata" directory in the workspace
        WorkspaceSettingsDialog* mDialog; ///< the settings dialog


        // Settings Items
        QList<WSI_Base*> mItems; ///< contains all settings items
        WSI_AppLocale* mAppLocale;
        WSI_AppDefaultMeasurementUnits* mAppDefMeasUnits;
        WSI_ProjectAutosaveInterval* mProjectAutosaveInterval;
        WSI_LibraryLocaleOrder* mLibraryLocaleOrder;
        WSI_LibraryNormOrder* mLibraryNormOrder;
        WSI_DebugTools* mDebugTools;
        WSI_Appearance* mAppearance;
};

#endif // WORKSPACESETTINGS_H
