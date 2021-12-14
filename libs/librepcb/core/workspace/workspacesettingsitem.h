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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class WorkspaceSettingsItem
 ******************************************************************************/

/**
 * @brief Base class for all workspace settings items
 *
 * For simple settings, see
 * ::librepcb::WorkspaceSettingsItem_GenericValue and
 * ::librepcb::WorkspaceSettingsItem_GenericValueList.
 *
 * @see ::librepcb::WorkspaceSettingsItem_GenericValue
 * @see ::librepcb::WorkspaceSettingsItem_GenericValueList
 */
class WorkspaceSettingsItem : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit WorkspaceSettingsItem(QObject* parent = nullptr) noexcept;
  WorkspaceSettingsItem(const WorkspaceSettingsItem& other) = delete;
  ~WorkspaceSettingsItem() noexcept;

  /**
   * @brief Restore default value
   *
   * @note Implementation must emit the #edited() signal.
   */
  virtual void restoreDefault() noexcept = 0;

  /**
   * @brief Load value from S-Expression node
   *
   * @param root        Root node of the settings file.
   * @param fileFormat  The file format of the settings file.
   *
   * @note Implementation must emit the #edited() signal.
   *
   * @note Implementation must be atomic, i.e. either the value must be loaded
   *       completely from file, or left at the old value (in case of errors).
   */
  virtual void load(const SExpression& root, const Version& fileFormat) = 0;

  /**
   * @brief Serialize the value into S-Expression node
   *
   * @param root  Root node of the settings file.
   */
  virtual void serialize(SExpression& root) const = 0;

  // Operator Overloadings
  WorkspaceSettingsItem& operator=(const WorkspaceSettingsItem& rhs) = delete;

signals:
  /**
   * @brief Signal to notify about changes of the settings value
   */
  void edited();
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
