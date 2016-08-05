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
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include<librepcbcommon/fileio/smartxmlfile.h>
#include "workspacesettings.h"
#include "../workspace.h"
#include "workspacesettingsdialog.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettings::WorkspaceSettings(const Workspace& workspace) throw (Exception) :
    QObject(nullptr), mXmlFilePath(workspace.getMetadataPath().getPathTo("settings.xml"))
{
    qDebug("Load workspace settings...");

    // load settings if the settings file exists
    QSharedPointer<XmlDomDocument> doc;
    if (mXmlFilePath.isExistingFile()) {
        SmartXmlFile file(mXmlFilePath, false, true);
        doc = file.parseFileAndBuildDomTree(true);
    } else {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        qInfo("Workspace settings file not found, default settings will be used.");
#else
        qWarning("Workspace settings file not found, default settings will be used.");
#endif
    }

    // load all settings
    XmlDomElement* root = doc.data() ? &doc->getRoot() : nullptr;
    loadSettingsItem(mAppLocale,                "app_locale",                   root);
    loadSettingsItem(mAppDefMeasUnits,          "app_default_meas_units",       root);
    loadSettingsItem(mProjectAutosaveInterval,  "project_autosave_interval",    root);
    loadSettingsItem(mAppearance,               "appearance",                   root);
    loadSettingsItem(mLibraryLocaleOrder,       "lib_locale_order",             root);
    loadSettingsItem(mLibraryNormOrder,         "lib_norm_order",               root);
    loadSettingsItem(mDebugTools,               "debug_tools",                  root);

    // load the settings dialog
    mDialog.reset(new WorkspaceSettingsDialog(*this));

    qDebug("Workspace settings successfully loaded!");
}

WorkspaceSettings::~WorkspaceSettings() noexcept
{
    mDialog.reset(); // the dialog must be deleted *before* any settings object!
    mItems.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WorkspaceSettings::restoreDefaults() noexcept
{
    foreach (WSI_Base* item, mItems) {
        item->restoreDefault();
    }
}

void WorkspaceSettings::applyAll() throw (Exception)
{
    foreach (WSI_Base* item, mItems) {
        item->apply();
    }

    saveToFile(); // can throw
}

void WorkspaceSettings::revertAll() noexcept
{
    foreach (WSI_Base* item, mItems) {
        item->revert();
    }
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WorkspaceSettings::showSettingsDialog() noexcept
{
    mDialog->exec(); // this is blocking
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template<typename T>
void WorkspaceSettings::loadSettingsItem(QScopedPointer<T>& member, const QString& xmlTagName,
                                         XmlDomElement* xmlRoot) throw (Exception)
{
    XmlDomElement* node = xmlRoot ? xmlRoot->getFirstChild(xmlTagName, false) : nullptr;
    member.reset(new T(xmlTagName, node));
    mItems.append(member.data());
}

void WorkspaceSettings::saveToFile() const throw (Exception)
{
    XmlDomDocument doc(*serializeToXmlDomElement());
    doc.setFileVersion(APP_VERSION_MAJOR);

    QScopedPointer<SmartXmlFile> file(SmartXmlFile::create(mXmlFilePath));
    file->save(doc, true); // can throw
}

XmlDomElement* WorkspaceSettings::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("workspace_settings"));
    foreach (WSI_Base* item, mItems) {
        root->appendChild(item->serializeToXmlDomElement());
    }
    return root.take();
}

bool WorkspaceSettings::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
