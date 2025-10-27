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

#ifndef LIBREPCB_CORE_CORPORATECHECK_H
#define LIBREPCB_CORE_CORPORATECHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../librarybaseelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Corporate;

/*******************************************************************************
 *  Class CorporateCheck
 ******************************************************************************/

/**
 * @brief The CorporateCheck class
 */
class CorporateCheck : public LibraryBaseElementCheck {
public:
  // Constructors / Destructor
  CorporateCheck() = delete;
  CorporateCheck(const CorporateCheck& other) = delete;
  explicit CorporateCheck(const Corporate& Corporate) noexcept;
  virtual ~CorporateCheck() noexcept;

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;

  // Operator Overloadings
  CorporateCheck& operator=(const CorporateCheck& rhs) = delete;

protected:  // Methods
  void checkInvalidImageFiles(MsgList& msgs) const;

private:  // Data
  const Corporate& mCorporate;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
