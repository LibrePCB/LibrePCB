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

#ifndef LIBREPCB_CORE_COMPONENTCHECK_H
#define LIBREPCB_CORE_COMPONENTCHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;

/*******************************************************************************
 *  Class ComponentCheck
 ******************************************************************************/

/**
 * @brief The ComponentCheck class
 */
class ComponentCheck : public LibraryElementCheck {
public:
  // Constructors / Destructor
  ComponentCheck() = delete;
  ComponentCheck(const ComponentCheck& other) = delete;
  explicit ComponentCheck(const Component& component) noexcept;
  virtual ~ComponentCheck() noexcept;

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const override;

  // Operator Overloadings
  ComponentCheck& operator=(const ComponentCheck& rhs) = delete;

protected:  // Methods
  void checkMissingPrefix(MsgList& msgs) const;
  void checkMissingDefaultValue(MsgList& msgs) const;
  void checkDuplicateSignalNames(MsgList& msgs) const;
  void checkMissingSymbolVariants(MsgList& msgs) const;
  void checkMissingSymbolVariantItems(MsgList& msgs) const;

private:  // Data
  const Component& mComponent;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
