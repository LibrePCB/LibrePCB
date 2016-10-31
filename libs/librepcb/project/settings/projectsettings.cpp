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
#include <librepcb/common/fileio/xmldomdocument.h>
#include <librepcb/common/fileio/xmldomelement.h>
#include "../project.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectSettings::ProjectSettings(Project& project, bool restore, bool readOnly, bool create) throw (Exception) :
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
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            XmlDomElement& root = doc->getRoot();

            // OK - XML file is open --> now load all settings

            // locale order
            for (XmlDomElement* node = root.getFirstChild("locale_order/locale", true, false);
                 node; node = node->getNextSibling("locale"))
            {
                mLocaleOrder.append(node->getText<QString>(true));
            }

            // norm order
            for (XmlDomElement* node = root.getFirstChild("norm_order/norm", true, false);
                 node; node = node->getNextSibling("norm"))
            {
                mNormOrder.append(node->getText<QString>(true));
            }
        }

        triggerSettingsChanged();

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
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
        XmlDomDocument doc(*serializeToXmlDomElement());
        mXmlFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ProjectSettings::checkAttributesValidity() const noexcept
{
    return true;
}

XmlDomElement* ProjectSettings::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("settings"));
    XmlDomElement* locale_order = root->appendChild("locale_order");
    foreach (const QString& locale, mLocaleOrder)
        locale_order->appendTextChild("locale", locale);
    XmlDomElement* norm_order = root->appendChild("norm_order");
    foreach (const QString& norm, mNormOrder)
        norm_order->appendTextChild("norm", norm);
    return root.take();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
