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

#ifndef LIBREPCB_CORE_ORGANIZATIONCHECK_H
#define LIBREPCB_CORE_ORGANIZATIONCHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../librarybaseelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Organization;

/*******************************************************************************
 *  Class OrganizationCheck
 ******************************************************************************/

/**
 * @brief The OrganizationCheck class
 */
class OrganizationCheck : public LibraryBaseElementCheck {
public:
  // Constructors / Destructor
  OrganizationCheck() = delete;
  OrganizationCheck(const OrganizationCheck& other) = delete;
  explicit OrganizationCheck(const Organization& Organization) noexcept;
  virtual ~OrganizationCheck() noexcept;

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;

  // Operator Overloadings
  OrganizationCheck& operator=(const OrganizationCheck& rhs) = delete;

private:  // Data
  const Organization& mOrganization;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
