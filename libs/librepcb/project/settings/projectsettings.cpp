/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "projectsettings.h"

#include "../project.h"

#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectSettings::ProjectSettings(Project& project, bool create)
  : QObject(nullptr), mProject(project) {
  qDebug() << "load settings...";

  // restore all default values
  restoreDefaults();

  // load settings from file
  if (!create) {
    QString fp = "project/settings.lp";
    SExpression root =
        SExpression::parse(mProject.getDirectory().read(fp),
                           mProject.getDirectory().getAbsPath(fp));

    // OK - file is open --> now load all settings

    // locale order
    foreach (const SExpression& node,
             root.getChild("library_locale_order").getChildren()) {
      mLocaleOrder.append(node.getChild("@0").getValue());
    }

    // norm order
    foreach (const SExpression& node,
             root.getChild("library_norm_order").getChildren()) {
      mNormOrder.append(node.getChild("@0").getValue());
    }
  }

  triggerSettingsChanged();

  qDebug() << "settings successfully loaded!";
}

ProjectSettings::~ProjectSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectSettings::restoreDefaults() noexcept {
  mLocaleOrder.clear();
  mNormOrder.clear();
}

void ProjectSettings::triggerSettingsChanged() noexcept {
  emit settingsChanged();
}

void ProjectSettings::save() {
  SExpression doc(
      serializeToDomElement("librepcb_project_settings"));  // can throw
  mProject.getDirectory().write("project/settings.lp",
                                doc.toByteArray());  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectSettings::serialize(SExpression& root) const {
  SExpression& locale_order = root.appendList("library_locale_order", true);
  foreach (const QString& locale, mLocaleOrder)
    locale_order.appendChild("locale", locale, true);
  SExpression& norm_order = root.appendList("library_norm_order", true);
  foreach (const QString& norm, mNormOrder)
    norm_order.appendChild("norm", norm, true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
