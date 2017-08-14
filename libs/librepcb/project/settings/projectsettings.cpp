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
#include "projectsettings.h"
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/domdocument.h>
#include "../project.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectSettings::ProjectSettings(Project& project, bool restore, bool readOnly, bool create) :
    QObject(nullptr), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/settings.xml")), mXmlFile(nullptr)
{
    qDebug() << "load settings...";
    Q_ASSERT(!(create && (restore || readOnly)));

    try
    {
        // restore all default values
        restoreDefaults();

        // try to create/open the XML file "settings.xml"
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mXmlFilepath);
        }
        else
        {
            mXmlFile = new SmartXmlFile(mXmlFilepath, restore, readOnly);
            std::unique_ptr<DomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            DomElement& root = doc->getRoot();

            // OK - XML file is open --> now load all settings

            // locale order
            foreach (const DomElement* node, root.getFirstChild("locale_order", true)->getChilds()) {
                mLocaleOrder.append(node->getText<QString>(true));
            }

            // norm order
            foreach (const DomElement* node, root.getFirstChild("norm_order", true)->getChilds()) {
                mNormOrder.append(node->getText<QString>(true));
            }
        }

        triggerSettingsChanged();
    }
    catch (...)
    {
        // free allocated memory and rethrow the exception
        delete mXmlFile;            mXmlFile = nullptr;
        throw;
    }

    qDebug() << "settings successfully loaded!";
}

ProjectSettings::~ProjectSettings() noexcept
{
    delete mXmlFile;            mXmlFile = nullptr;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ProjectSettings::restoreDefaults() noexcept
{
    mLocaleOrder.clear();
    mNormOrder.clear();
}

void ProjectSettings::triggerSettingsChanged() noexcept
{
    emit settingsChanged();
}

bool ProjectSettings::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/settings.xml"
    try
    {
        DomDocument doc(*serializeToDomElement("settings"));
        mXmlFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ProjectSettings::serialize(DomElement& root) const
{
    DomElement* locale_order = root.appendChild("locale_order");
    foreach (const QString& locale, mLocaleOrder)
        locale_order->appendTextChild("locale", locale);
    DomElement* norm_order = root.appendChild("norm_order");
    foreach (const QString& norm, mNormOrder)
        norm_order->appendTextChild("norm", norm);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
