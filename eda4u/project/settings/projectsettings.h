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

#ifndef PROJECT_PROJECTSETTINGS_H
#define PROJECT_PROJECTSETTINGS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/fileio/if_xmlserializableobject.h>
#include <eda4ucommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SmartXmlFile;

namespace project {
class Project;
}

/*****************************************************************************************
 *  Class ProjectSettings
 ****************************************************************************************/

namespace project {

/**
 * @brief The ProjectSettings class
 *
 * @author ubruhin
 * @date 2015-03-22
 */
class ProjectSettings final : public QObject, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ProjectSettings(Project& project, bool restore, bool readOnly, bool create) throw (Exception);
        ~ProjectSettings() noexcept;

        // Getters: General
        Project& getProject() const noexcept {return mProject;}

        // Getters: Settings
        QStringList getLocaleOrder(bool useWorkspaceSettings) const noexcept;
        QStringList getNormOrder(bool useWorkspaceSettings) const noexcept;

        // Setters: Settings
        void setLocaleOrder(const QStringList& locales) noexcept {mLocaleOrder = locales;}
        void setNormOrder(const QStringList& norms) noexcept {mNormOrder = norms;}

        // General Methods
        void restoreDefaults() noexcept;
        void triggerSettingsChanged() noexcept;
        bool save(bool toOriginal, QStringList& errors) noexcept;
        void showSettingsDialog(QWidget* parent = nullptr);


    signals:

        void settingsChanged();


    private:

        // make some methods inaccessible...
        ProjectSettings();
        ProjectSettings(const ProjectSettings& other);
        ProjectSettings& operator=(const ProjectSettings& rhs);

        // Private Methods

        /**
         * @copydoc IF_XmlSerializableObject#checkAttributesValidity()
         */
        bool checkAttributesValidity() const noexcept;

        /**
         * @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
         */
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


        // General
        Project& mProject; ///< a reference to the Project object (from the ctor)
        FilePath mLibraryPath; ///< the "lib" directory of the project

        // File "core/settings.xml"
        FilePath mXmlFilepath;
        SmartXmlFile* mXmlFile;


        // All Settings
        QStringList mLocaleOrder; ///< The list of locales (like "de_CH") in the right order
        QStringList mNormOrder; ///< the list of norms in the right order
};

} // namespace project

#endif // PROJECT_PROJECTSETTINGS_H
