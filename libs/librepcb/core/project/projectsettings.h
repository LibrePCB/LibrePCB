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

#ifndef LIBREPCB_CORE_PROJECTSETTINGS_H
#define LIBREPCB_CORE_PROJECTSETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;
class SExpression;
class Version;

/*******************************************************************************
 *  Class ProjectSettings
 ******************************************************************************/

/**
 * @brief The ProjectSettings class
 */
class ProjectSettings final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectSettings() = delete;
  ProjectSettings(const ProjectSettings& other) = delete;
  explicit ProjectSettings(Project& project);
  ~ProjectSettings() noexcept;

  // Getters: Settings
  const QStringList& getLocaleOrder() const noexcept { return mLocaleOrder; }
  const QStringList& getNormOrder() const noexcept { return mNormOrder; }

  // Setters: Settings
  void setLocaleOrder(const QStringList& locales) noexcept {
    mLocaleOrder = locales;
  }
  void setNormOrder(const QStringList& norms) noexcept { mNormOrder = norms; }

  // General Methods
  void restoreDefaults() noexcept;
  void triggerSettingsChanged() noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  ProjectSettings& operator=(const ProjectSettings& rhs) = delete;

signals:
  void settingsChanged();

private:
  // General
  Project& mProject;  ///< a reference to the Project object (from the ctor)

  // All Settings
  QStringList
      mLocaleOrder;  ///< The list of locales (like "de_CH") in the right order
  QStringList mNormOrder;  ///< the list of norms in the right order
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
