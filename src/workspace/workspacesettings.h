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
#include "../common/units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;
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
 */
class WorkspaceSettings final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WorkspaceSettings(Workspace* workspace, const QString& metadataPath);
        ~WorkspaceSettings();


        // Getters: General

        /**
         * @brief Get the absolute filepath to a file in the ".metadata" directory
         *
         * @param filename  The name of a file in the .metadata directory.
         *                  The filename can contain subfolders.
         *                  "settings.ini" is the default filename.
         *
         * @return The absolute filepath (with native separators) to the specified file
         */
        QString getFilepath(const QString& filename = "settings.ini") const;


        // Getters: Settings attributes

        /**
         * @brief Get the application's locale (for translation and localization)
         *
         * @return see #mAppLocaleName
         *
         * @note You should not use this string to create a QLocale object for
         * localization purposes! After the language is changed in the settings dialog,
         * this method will return the new language, even if it isn't applied until
         * the application is restarted! So you will get the wrong language. Simply use
         * the default constructor of QLocale instead, as the default locale was set on
         * application startup (in WorkspaceSettings#WorkspaceSettings()).
         */
        const QString& getAppLocaleName() const {return mAppLocaleName;}

        /**
         * @brief Get the application's default measurement unit
         *
         * @return The default measurement unit (see Length::MeasurementUnit)
         */
        Length::MeasurementUnit getAppDefMeasUnit() const {return mAppDefMeasUnit;}


        // Setters: Settings attributes

        /**
         * @brief Set the application's locale (for translation and localization)
         *
         * @param name      see #mAppLocaleName
         */
        void setAppLocaleName(const QString& name);

        /**
         * @brief Set the application's default measurement unit
         *
         * @param unit      The new unit (see Length::MeasurementUnit)
         */
        void setAppDefMeasUnit(Length::MeasurementUnit unit);


    public slots:

        // Public Slots

        /**
         * @brief Open the workspace settings dialog
         *
         * @note The dialog is application modal, so this method is blocking while the
         * dialog is open. This method will not return before the dialog is closed.
         */
        void showSettingsDialog();


    private:

        // make some methods inaccessible...
        WorkspaceSettings();
        WorkspaceSettings(const WorkspaceSettings& other);
        WorkspaceSettings& operator=(const WorkspaceSettings& rhs);


        // General Methods

        /**
         * @brief Load all settings from the workspace and store them in the member variables
         *
         * This is done in the constructor WorkspaceSettings::WorkspaceSettings()
         */
        void load();


        // General Attributes
        Workspace* mWorkspace; ///< a pointer to the Workspace object
        QDir mMetadataDir; ///< a QDir object for the ".metadata" directory in the workspace
        QList<QTranslator*> mInstalledTranslators; ///< see constructor/destructor code


        // Settings Attributes

        /**
         * @brief The locale name for the whole application (for translation and localization)
         *
         * Examples:
         *  - QString("de_CH") for German/Switzerland
         *  - QString("") or QString() means "use system locale"
         *
         * Default: QString()
         */
        QString mAppLocaleName;

        /**
         * @brief The default measurement unit for the whole application
         *
         * Default: Length::millimeters
         */
        Length::MeasurementUnit mAppDefMeasUnit;
};

#endif // WORKSPACESETTINGS_H
