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

#ifndef LIBREPCB_EDITOR_ORGANIZATIONPCBDESIGNRULESMODEL_H
#define LIBREPCB_EDITOR_ORGANIZATIONPCBDESIGNRULESMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/org/organizationpcbdesignrules.h>

#include <QtCore>

#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Organization;

namespace editor {

class UndoStack;

/*******************************************************************************
 *  Class OrganizationPcbDesignRulesModel
 ******************************************************************************/

/**
 * @brief The OrganizationPcbDesignRulesModel class
 */
class OrganizationPcbDesignRulesModel final
  : public QObject,
    public slint::Model<ui::OrganizationPcbDesignRulesData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // OrganizationPcbDesignRulesModel() = delete;
  OrganizationPcbDesignRulesModel(
      const OrganizationPcbDesignRulesModel& other) = delete;
  explicit OrganizationPcbDesignRulesModel(QObject* parent = nullptr) noexcept;
  ~OrganizationPcbDesignRulesModel() noexcept;

  // General Methods
  void setReferences(
      Organization* organization, UndoStack* stack,
      std::function<void(OrganizationPcbDesignRules&)> editCallback) noexcept;
  void addItem() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::OrganizationPcbDesignRulesData> row_data(
      std::size_t i) const override;
  void set_row_data(
      std::size_t i,
      const ui::OrganizationPcbDesignRulesData& data) noexcept override;

  // Operator Overloadings
  OrganizationPcbDesignRulesModel& operator=(
      const OrganizationPcbDesignRulesModel& rhs) = delete;

private:
  void refresh() noexcept;
  void trigger(int index, const Uuid& uuid,
               ui::OrganizationPcbDesignRulesAction a) noexcept;
  void setList(const QVector<OrganizationPcbDesignRules>& list);
  QString askForName(const QString& defaultValue) const;

private:
  QPointer<Organization> mOrganization;
  QPointer<UndoStack> mUndoStack;
  std::function<void(OrganizationPcbDesignRules&)> mEditCallback;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
