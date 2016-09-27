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

#ifndef LIBREPCB_WORKSPACESETTINGS_H
#define LIBREPCB_WORKSPACESETTINGS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/fileio/filepath.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

// All Settings Classes
#include "items/wsi_applocale.h"
#include "items/wsi_appdefaultmeasurementunits.h"
#include "items/wsi_projectautosaveinterval.h"
#include "items/wsi_librarylocaleorder.h"
#include "items/wsi_librarynormorder.h"
#include "items/wsi_debugtools.h"
#include "items/wsi_appearance.h"
#include "items/wsi_repositories.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

class Workspace;
class WorkspaceSettingsDialog;

/*****************************************************************************************
 *  Class WorkspaceSettings
 ****************************************************************************************/

/**
 * @brief The WorkspaceSettings class manages all workspace related settings
 *
 * The ".metadata/settings.xml" file in a workspace is used to store workspace related
 * settings. This class is an interface to these settings. A WorkspaceSettings object is
 * created in the constructor of the Workspace object.
 *
 * This class also provides a graphical dialog to show and edit all these settings. For
 * this purpose, the WorkspaceSettingsDialog class is used. It can be shown by calling
 * the slot #showSettingsDialog().
 *
 * @author ubruhin
 * @date 2014-07-12
 *
 * @todo Maybe use XML files instead of INI to save all workspace settings...
 */
class WorkspaceSettings final : public QObject, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WorkspaceSettings() = delete;
        WorkspaceSettings(const WorkspaceSettings& other) = delete;
        explicit WorkspaceSettings(const Workspace& workspace) throw (Exception);
        ~WorkspaceSettings() noexcept;


        // Getters: Settings Items
        WSI_AppLocale& getAppLocale() const noexcept {return *mAppLocale;}
        WSI_AppDefaultMeasurementUnits& getAppDefMeasUnits() const noexcept {return *mAppDefMeasUnits;}
        WSI_ProjectAutosaveInterval& getProjectAutosaveInterval() const noexcept {return *mProjectAutosaveInterval;}
        WSI_Appearance& getAppearance() const noexcept {return *mAppearance;}
        WSI_LibraryLocaleOrder& getLibLocaleOrder() const noexcept {return *mLibraryLocaleOrder;}
        WSI_LibraryNormOrder& getLibNormOrder() const noexcept {return *mLibraryNormOrder;}
        WSI_Repositories& getRepositories() const noexcept {return *mRepositories;}
        WSI_DebugTools& getDebugTools() const noexcept {return *mDebugTools;}


        // General Methods
        void restoreDefaults() noexcept;
        void applyAll() throw (Exception);
        void revertAll() noexcept;

        // Operator Overloadings
        WorkspaceSettings& operator=(const WorkspaceSettings& rhs) = delete;


    public slots:

        /**
         * @brief Open the workspace settings dialog
         *
         * @note The dialog is application modal, so this method is blocking while the
         * dialog is open. This method will not return before the dialog is closed.
         */
        void showSettingsDialog() noexcept;


    private: // Methods

        template<typename T>
        void loadSettingsItem(QScopedPointer<T>& member, const QString& xmlTagName,
                              XmlDomElement* xmlRoot) throw (Exception);
        void saveToFile() const throw (Exception);
        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;
        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


    private: // Data

        // General Attributes
        FilePath mXmlFilePath; ///< path to the ".metadata/settings.xml" file
        QScopedPointer<WorkspaceSettingsDialog> mDialog; ///< the settings dialog

        // Settings Items
        QList<WSI_Base*> mItems; ///< contains all settings items
        QScopedPointer<WSI_AppLocale> mAppLocale;
        QScopedPointer<WSI_AppDefaultMeasurementUnits> mAppDefMeasUnits;
        QScopedPointer<WSI_ProjectAutosaveInterval> mProjectAutosaveInterval;
        QScopedPointer<WSI_Appearance> mAppearance;
        QScopedPointer<WSI_LibraryLocaleOrder> mLibraryLocaleOrder;
        QScopedPointer<WSI_LibraryNormOrder> mLibraryNormOrder;
        QScopedPointer<WSI_Repositories> mRepositories;
        QScopedPointer<WSI_DebugTools> mDebugTools;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACESETTINGS_H
