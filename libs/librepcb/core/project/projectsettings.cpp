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

#include "../serialization/sexpression.h"
#include "project.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectSettings::ProjectSettings(Project& project)
  : QObject(nullptr), mProject(project) {
  restoreDefaults();
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

void ProjectSettings::serialize(SExpression& root) const {
  root.ensureLineBreak();
  {
    SExpression& node = root.appendList("library_locale_order");
    foreach (const QString& locale, mLocaleOrder) {
      node.ensureLineBreak();
      node.appendChild("locale", locale);
    }
    node.ensureLineBreak();
  }

  root.ensureLineBreak();
  {
    SExpression& node = root.appendList("library_norm_order");
    foreach (const QString& norm, mNormOrder) {
      node.ensureLineBreak();
      node.appendChild("norm", norm);
    }
    node.ensureLineBreak();
  }

  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
